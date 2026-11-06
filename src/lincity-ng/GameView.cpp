/* ---------------------------------------------------------------------- *
 * src/lincity-ng/GameView.cpp
 * This file is part of Lincity-NG.
 *
 * Copyright (C) 2005      Wolfgang Becker <uafr@gmx.de>
 * Copyright (C) 2024-2025 David Bears <dbear4q@gmail.com>
 * Copyright (C) 2026      Marc Young <myoung008@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
** ---------------------------------------------------------------------- */

#include "GameView.hpp"
#include "lincity/street_routes.hpp"
#include "NightCrops.hpp"
#include "lincity/building_visual_state.hpp"
#include "lincity/modules/health_centre.hpp"
#include "lincity/modules/firestation.hpp"
#include "lincity/modules/residence.hpp"
#include "lincity/traffic_era.hpp"

#include <SDL3/SDL.h>                     // for SDL_Scancode, SDL_BUTTON_LEFT
#include <SDL3_image/SDL_image.h>         // for IMG_Load
#include <assert.h>                       // for assert
#include <fmt/base.h>                     // for println
#include <fmt/format.h>                   // for format
#include <libxml++/parsers/textreader.h>  // for TextReader
#include <libxml++/ustring.h>             // for ustring
#include <cstdio>                         // for stderr
#include <cmath>                          // for sqrt, fabs, fabsf, floorf
#include <cstddef>                        // for size_t, NULL
#include <exception>                      // for exception
#include <functional>                     // for bind, _1, function
#include <iostream>                       // for operator<<, basic_ostream
#include <iterator>                       // for advance
#include <list>                           // for _List_iterator, list, opera...
#include <map>                            // for map, operator!=
#include <sstream>                        // for basic_stringstream
#include <stdexcept>                      // for runtime_error
#include <utility>                        // for pair
#include <vector>                         // for vector
#include <fmt/std.h> // IWYU pragma: keep

#include "Config.hpp"                     // for getConfig, Config
#include "Dialog.hpp"                     // for blockingDialogIsOpen
#include "Game.hpp"                       // for Game
#include "MiniMap.hpp"                    // for MiniMap, getMiniMap
#include "Mps.hpp"                        // for MpsMap
#include "Util.hpp"                       // for getButton, getParagraph
#include "gui/Button.hpp"                 // for Button
#include "gui/Color.hpp"                  // for Color
#include "gui/ComponentFactory.hpp"       // for IMPLEMENT_COMPONENT_FACTORY
#include "gui/Desktop.hpp"                // for Desktop
#include "gui/Event.hpp"                  // for Event
#include "gui/Painter.hpp"                // for Painter
#include "gui/Paragraph.hpp"              // for Paragraph
#include "gui/Rect2D.hpp"                 // for Rect2D
#include "gui/Signal.hpp"                 // for Signal
#include "gui/Texture.hpp"                // for Texture
#include "gui/TextureManager.hpp"         // for TextureManager, texture_man...
#include "lincity-ng/UserOperation.hpp"   // for UserOperation
#include "lincity/all_buildings.hpp"      // for GROUP_WATER_BUL_COST, GROUP...
#include "lincity/groups.hpp"             // for GROUP_DESERT, GROUP_WATER
#include "lincity/lin-city.hpp"           // for FLAG_POWER_CABLES_0, FLAG_P...
#include "lincity/lintypes.hpp"           // for ConstructionGroup, Construc...
#include "lincity/messages.hpp"           // for Message
#include "lincity/modules/tile.hpp"       // for TileConstructionGroup, bare...
#include "lincity/transport.hpp"          // for BRIDGE_FACTOR
#include "lincity/world.hpp"              // for Map, World, MapTile, Ground
#include "util/gettextutil.hpp"           // for _
#include "util/xmlutil.hpp"               // for xmlParse, unexpectedXmlAttr...

using namespace std::placeholders;


const int scale3d = 128; // guestimate value for good looking 3d view;

const float GameView::defaultTileWidth = 128;
const float GameView::defaultTileHeight = 64;
const float GameView::defaultZoom = 1.0;    // fastest drawing

GameView::GameView() {
    loaderThread = 0;
    keyScrollState = SCROLL_NONE;
    mouseScrollState = SCROLL_NONE;
    remaining_images = 0;
    textures_ready = false;
}

GameView::~GameView() {
    stopThread = true;
    SDL_WaitThread( loaderThread, NULL );
}

//Static function to use with SDL_CreateThread
int GameView::gameViewThread( void* data )
{
    GameView* gv = (GameView*) data;
    gv->preReadImages();
    gv->textures_ready = true;
    //keep thread alive as long as there are SDL_Surfaces
    while(!gv->stopThread && gv->remaining_images!=0)
    {   SDL_Delay(100);}
    return 0;
}

void
GameView::parse(xmlpp::TextReader& reader) {
  while(reader.move_to_next_attribute()) {
    xmlpp::ustring name = reader.get_name();
    xmlpp::ustring value = reader.get_value();
    if(parseAttribute(reader));
    else
      unexpectedXmlAttribute(reader);
  }
  reader.move_to_element();

  blankGraphicsInfo.texture = readTexture( "blank.png" );
  blankGraphicsInfo.x = blankGraphicsInfo.texture->getWidth() / 2;
  blankGraphicsInfo.y = blankGraphicsInfo.texture->getHeight();

  stopThread = false;
  loaderThread = SDL_CreateThread( gameViewThread, "Loader", this );

  //GameView is resizable
  setFlags(FLAG_RESIZABLE);

  //start at location from savegame
  zoom = defaultZoom;
  tileWidth = defaultTileWidth * zoom;
  tileHeight = defaultTileHeight * zoom;

  mouseInGameView = false;
  dragging = false;
  leftButtonDown = false;
  roadDragging = false;
  ctrDrag = false;
  areaBulldoze = false;
  startRoad = MapPoint(0, 0);
  tileUnderMouse = MapPoint(0, 0);
  hideHigh = false;
  showTerrainHeight = false;
  cursorSize = 0;

  mapOverlay = overlayNone;
  buttonsConnected = false;
  lastStatusMessage = "";
  refreshMap = true;
}

void
GameView::setGame(Game *game) {
      this->game = game;
    hideHigh=false; mapOverlay=overlayNone; updateBuildingViewIndicator(); setMapDirty();
  game->getMiniMap().mapChanged.connect([this](){
    if(mapOverlay != overlayNone) {
      setDirty();
      setMapDirty();
    }
  });
}

World&
GameView::getWorld() const {
  return game->getWorld();
}

UserOperation *
GameView::getUserOperation() const {
  return &game->getUserOperation();
}

/*
 * Connect to GUI-Buttons.
 */
void GameView::connectButtons(){
    if( buttonsConnected ){
        return;
    }
    buttonsConnected = true;
    Component* root = this;
    while( root->getParent() ){
        root = root->getParent();
    }
    updateBuildingViewIndicator();
    Button* button = getButton( *root, "hideHighBuildings" );
    button->clicked.connect(std::bind(&GameView::buttonClicked, this, _1));

    button = getButton( *root, "showTerrainHeight" );
    button->clicked.connect(std::bind(&GameView::buttonClicked, this, _1));

    button = getButton( *root, "mapOverlay" );
    button->clicked.connect(std::bind(&GameView::buttonClicked, this, _1));
}

/*
 * Handle Clicks on Button.
 */
void GameView::updateBuildingViewIndicator(){Component* root=this;while(root->getParent())root=root->getParent();if(auto* b=dynamic_cast<Button*>(root->findComponent("ShowBuildingsButton")))b->setCaptionText(hideHigh?"Buildings hidden - show":"Buildings visible - hide");}
void GameView::buttonClicked( Button* button ){
    std::string name = button->getName();
    if(name=="ShowBuildingsButton"){hideHigh=!hideHigh;updateBuildingViewIndicator();setDirty();setMapDirty();return;}
    if( name == "hideHighBuildings" ){
        hideHigh = !hideHigh;
        updateBuildingViewIndicator();
        setDirty();
        setMapDirty();
        return;
    }
    if( name == "showTerrainHeight" ){
        if(getWorld().map.alt_step != 0) {
            showTerrainHeight = !showTerrainHeight;
        } else { // map is completely flat
            showTerrainHeight = false;
        }
        setDirty();
        return;
    }
    if( name == "mapOverlay" ){
        mapOverlay = (mapOverlay + 1) % (overlayMAX + 1);
        setDirty();
        setMapDirty();
        return;
    }
    std::cerr << "GameView::buttonClicked# Unhandled Button '" << name <<"',\n";
}

/*
 * size in Tiles of marking under Cursor
 * atm 0 is an outlined Version of size 1.
 */
void GameView::setCursorSize( int size )
{
    if( size != cursorSize )
    {
        cursorSize = size;
        setDirty();
    }
}

/*
 * evaluate main_screen_originx and main_screen_originy
 */
void GameView::readOrigin( bool redraw /* = true */ ) {
  show(getWorld().map.recentPoint, redraw);
}

/*
 * set main_screen_originx and main_screen_originy
 */
void GameView::writeOrigin() {
  getWorld().map.recentPoint = getCenter().x;
}

/*
 *  Get Tile in Center of Screen.
 */
MapPoint GameView::getCenter(){
    Vector2 center( getWidth() / 2, getHeight() / 2 );
    return getTile( center );
}

/*
 * Adjust the Zoomlevel.
 */
void GameView::setZoom(float newzoom){
    MapPoint centerTile  = getCenter();

    //if ( newzoom < .0625 ) return;
    if ( newzoom < .0312 ) return;
    if ( newzoom > 4 ) return;

    zoom = newzoom;

    // fix rounding errors...
    if(fabs(zoom - 1.0) < .01)
    {   zoom = 1;}

    tileWidth = defaultTileWidth * zoom;
    tileHeight = defaultTileHeight * zoom;

    //Show the Center
    show( centerTile );
}

void GameView::zoomMouse(float factor, Vector2 mousepos) {
    float newzoom = zoom * factor;

    //if ( newzoom < .0625 ) return;
    if (newzoom < .0312) {
        newzoom = .0312;
        factor = newzoom / zoom;
    }
    if(newzoom > 4) {
        newzoom = 4;
        factor = newzoom / zoom;
    }
    if(fabs(newzoom - 1.0) < .01) {
      // fix rounding errors...
      newzoom = 1;
      factor = newzoom / zoom;
    }

    zoom = newzoom;

    tileWidth = defaultTileWidth * zoom;
    tileHeight = defaultTileHeight * zoom;

    viewport = (viewport + mousepos) * factor - mousepos;
    constrainViewportPosition(true);

    viewportUpdated();
    setDirty();
}

/* set Zoomlevel to 100% */
void GameView::resetZoom(){
    setZoom( defaultZoom );
}

/* increase Zoomlevel */
void GameView::zoomIn(){
    setZoom( zoom * sqrt( 2.f ) );
}

/** decrease Zoomlevel */
void GameView::zoomOut(){
    setZoom( zoom / sqrt( 2.f ) );
}

/**
 *  Show City Tile(x/y) by centering the screen
 *  redraw = false is used on initialisation.
 */
void GameView::show( MapPoint map , bool redraw /* = true */ )
{
    Vector2 center;
    center.x = ( map.x - map.y ) * ( tileWidth / 2 );
    center.y = ( map.x + map.y ) * ( tileHeight / 2 ) + ( tileHeight / 2 );
    if( redraw ){
        viewport.x = center.x - ( getWidth() / 2 );
        viewport.y = center.y - ( getHeight() / 2 );
        viewportUpdated();
        setDirty();
    } else { //on startup getWidth is 0.
        viewport.x = center.x - ( getConfig()->videoX.get() / 2 );
        viewport.y = center.y - ( getConfig()->videoY.get() / 2 );
    }
}

/*
 * Loads Texture from filename, Returns Pointer to Texture
 * or Null if no file found. NOT THREADSAFE
 */

Texture* GameView::readTexture(const std::filesystem::path& filename) {
  Texture* currentTexture;
  try {
    currentTexture = texture_manager->load(
      std::filesystem::path("images") / "tiles" / filename);
  } catch(std::exception& err) {
    std::cerr << filename << " GameView::readTexture# missing: "
      << err.what() << "\n";
    return 0;
  }
  return currentTexture;
}

/*
 * Loads Image from filename, Returns Pointer to Image
 * or Null if no file found. IS THREADSAFE
 */
SDL_Surface* GameView::readImage(const std::filesystem::path& filename) {
  SDL_Surface* currentImage = IMG_Load((getConfig()->appDataDir.get() / "images" /
    "tiles" / filename).string().c_str());
  if( !currentImage ) {
    std::cerr << "GameView::readImage# Could not load image " << filename << "\n";
  }
  return currentImage;
}

/**
 * preload all images and with X and Y offsets.
 * from data/images/tiles/images.xml
 *
 *images and offsets are appended to ConstructionGroup::graphicVectorInfo see lintypes.h
 *the resourceID strings are hard coded in all_modules.cpp and match the names of the sounds
 *
 * some images contain the x-Coordinate of the
 * middle of the Building in Case the Image is asymetric,
 * eg. a high tower with a long shadow to the right
 *
 *  Some of the Image to Texture Conversion seems not to be threadsave
 *  in OpenGL, so load just Images and convert them to Textures on
 *  demand in the main Tread.
**/

void
GameView::preReadImages(void) {
  // TODO: Improve the format of images.xml so it doesn't rely on the order of
  //       elements.

  std::filesystem::path fullpath = getConfig()->appDataDir.get() /
    "images" / "tiles" / "images.xml";
  xmlpp::TextReader reader(fullpath.string());
  if(!reader.read())
    throw std::runtime_error(fmt::format("file is empty: {}", fullpath));
  while(reader.get_node_type() != xmlpp::TextReader::NodeType::Element) {
    if(!reader.next())
      throw std::runtime_error(
        fmt::format("file doesn't contain XML data: {}", fullpath));
  }

  ResourceGroup *resourceGroup = 0;
  int resourceID_level = 0;
  std::string key;

  if(!reader.is_empty_element())
  while(reader.read()) {
    if(reader.get_node_type() != xmlpp::TextReader::NodeType::Element) {
      continue;
    }
    const xmlpp::ustring element = reader.get_name();

    if(element == "resourceID") {
      std::string name;

      while(reader.move_to_next_attribute()) {
        xmlpp::ustring aname = reader.get_name();
        xmlpp::ustring value = reader.get_value();
        if(aname == "name")
          name = xmlParse<std::string>(value);
        else
          unexpectedXmlAttribute(reader);
      }
      reader.move_to_element();

      if(name.empty())
        missingXmlAttribute(reader, "name");

      if(resourceGroup)
        resourceGroup->images_loaded = true;

      if(ResourceGroup::resMap.count(name)) {
        resourceGroup = ResourceGroup::resMap[name];
        resourceID_level = reader.get_depth();
        if(resourceGroup->images_loaded) {
          fmt::println(stderr,
            "warning: duplicate resourceID in images.xml: {}", name);
          assert(false);
        }
      }
      else {
        fmt::println(stderr,
          "warning: unknown resourceID in images.xml: {}", name);
        resourceGroup = 0;
        resourceID_level = 0;
      }
    }
    //check if we are still inside context of last resorceID
    if(reader.get_depth() < resourceID_level-1) {
      resourceGroup->images_loaded = true;
      resourceGroup = 0;
      resourceID_level = 0;
    }
    if(element == "image") {
      int xmlX = 64;
      int xmlY = 32;
      bool hasX = false, hasY = false;

      while(reader.move_to_next_attribute()) {
        xmlpp::ustring name = reader.get_name();
        xmlpp::ustring value = reader.get_value();
        if(name == "file")
          key = xmlParse<std::string>(value);
        else if(name == "x") {
          xmlX = xmlParse<int>(value);
          hasX = true;
        }
        else if(name == "y") {
          xmlY = xmlParse<int>(value);
          hasY = true;
        }
        else
          unexpectedXmlAttribute(reader);
      }
      reader.move_to_element();

      if(resourceID_level && resourceGroup) {
        resourceGroup->growGraphicsInfoVector();
        GraphicsInfo *graphicsInfo = &(resourceGroup->graphicsInfoVector.back());
        graphicsInfo->nightKey = resourceGroup->resourceID + ":" + std::to_string(resourceGroup->graphicsInfoVector.size()-1);
        graphicsInfo->image = readImage( key );
        if(!graphicsInfo->image) {
          fmt::println(stderr, "error: failed to read image {}", key);
        }
        assert(hasX && hasY);
        if(!hasX) xmlX = int(graphicsInfo->image->w/2);
        if(!hasY) xmlY = int(graphicsInfo->image->h);
        graphicsInfo->x = xmlX;
        graphicsInfo->y = xmlY;
        ++remaining_images;
      }
      key.clear();
    }
  }

  while(reader.next()) {
    if(reader.get_node_type() != xmlpp::TextReader::NodeType::Element)
      continue;
    unexpectedXmlElement(reader);
  }

  if(resourceGroup)
  {
      resourceGroup->images_loaded = true;
      resourceGroup = 0;
  }
}


/*
 * Scroll the map.
 */
void GameView::scroll(float elapsedTime)
{
    int scrollState = keyScrollState | mouseScrollState;
    if( scrollState == SCROLL_NONE ) {
        return;
    }

    //TODO: scroll speed should be configurable
    // The sqrt(zoom) makes it feel like the same speed at different zoom
    // levels.
    float amt = (elapsedTime * 1000) * 0.5 * sqrt(zoom);
    Vector2 dir = Vector2(0,0);

    if( keyScrollState & SCROLL_SHIFT_ALL ) {
        amt *= 4;
    }

    if( scrollState & SCROLL_UP_ALL ) {
        dir.y -= 1;
    }
    if( scrollState & SCROLL_DOWN_ALL ) {
        dir.y += 1;
    }
    if( scrollState & SCROLL_LEFT_ALL ) {
        dir.x -= 1;
    }
    if( scrollState & SCROLL_RIGHT_ALL ) {
        dir.x += 1;
    }

    if(dir == Vector2(0,0)) return;

    // The sqrt((float)tileWidth / tileHeight) makes vertical/horizonal
    // scrolling feel like the same speed. Surprisingly, without the square
    // root, it doesn't feel right.
    float norm = hypot(dir.x * sqrt((float)tileWidth / tileHeight), dir.y);
    // This makes diagonal scrolling parallel to map components.
    dir.x *= (float)tileWidth / tileHeight;
    viewport += dir * amt / norm;
    constrainViewportPosition(false);

    viewportUpdated();
    setDirty();
}

bool GameView::constrainViewportPosition(bool useScrollCorrection) {
  //If the centre of the Screen is not Part of the city
  //adjust viewport so it is.
  if(useScrollCorrection)
    viewport += scrollCorrection * zoom;
  Vector2 center = viewport + (Vector2(getWidth(), getHeight()) / 2);
  Vector2 centerTile = Vector2(
    center.y / tileHeight + center.x / tileWidth,
    center.y / tileHeight - center.x / tileWidth
  );
  bool outside = false;
  int mapSize = getWorld().map.len();
  if(centerTile.x < 1) {
      centerTile.x = 1;
      outside = true;
  }
  else if(centerTile.x > mapSize - 1) {
      centerTile.x = mapSize - 1;
      outside = true;
  }
  if(centerTile.y < 1) {
      centerTile.y = 1;
      outside = true;
  }
  else if(centerTile.y > mapSize - 1) {
      centerTile.y = mapSize - 1;
      outside = true;
  }

  if(outside) {
      Vector2 vpOld = viewport;
      center.x = ( centerTile.x - centerTile.y ) * tileWidth / 2;
      center.y = ( centerTile.x + centerTile.y ) * tileHeight / 2;
      viewport = center - (Vector2(getWidth(), getHeight()) / 2);
      if(useScrollCorrection)
        scrollCorrection = (vpOld - viewport) / zoom;
      else
        scrollCorrection = Vector2(0,0);
      viewportUpdated();
      setDirty();
      return true;
  }
  else {
    scrollCorrection = Vector2(0,0);
    return false;
  }
}

void GameView::beginDistrictZone(int type) {
  districtMode = true; districtDrag = false; districtType = type;
  roadDragging = false; areaBulldoze = false; leftButtonDown = false;
  printStatusMessage(type == 0 ? std::string("Drag a rectangle on the map for the mayor's work zone. Esc cancels.")
    : std::string("Drag a rectangle on the map for the new ") + CityEvolution::districtTypeName(type) + " district. It may cover existing buildings. Esc cancels.");
  setDirty();
}

void
GameView::event(const Event& event) {
  // District zone mode: a left-button drag selects the rectangle; nothing is built or bulldozed.
  if(districtMode) {
    if(event.type == Event::KEYDOWN && event.scancode == SDL_SCANCODE_ESCAPE) {
      districtMode = false; districtDrag = false; printStatusMessage("District zone cancelled."); setDirty(); return;
    }
    if(event.type == Event::MOUSEBUTTONDOWN && event.mousebutton == SDL_BUTTON_LEFT && event.inside) {
      districtStart = districtEnd = getTile(event.mousepos); districtDrag = true; setDirty(); return;
    }
    if(event.type == Event::MOUSEMOTION && districtDrag) { districtEnd = getTile(event.mousepos); setDirty(); }
    if(event.type == Event::MOUSEBUTTONUP && event.mousebutton == SDL_BUTTON_LEFT && districtDrag) {
      districtEnd = getTile(event.mousepos);
      districtDrag = false; districtMode = false;
      auto& world = getWorld();
      if(districtType == 0) {
        game->mayorZoneDrawn(districtStart.x, districtStart.y, districtEnd.x, districtEnd.y);
      } else if(world.evolution.addDistrict(world, districtType, districtStart.x, districtStart.y, districtEnd.x, districtEnd.y, "")) {
        game->districtDrawn(int(world.evolution.districts.size()) - 1);
      } else {
        printStatusMessage("That zone is too small or you already have 12 districts. Drag at least 2x2 tiles.");
      }
      setDirty(); return;
    }
    if(event.type == Event::MOUSEBUTTONUP || event.type == Event::MOUSEBUTTONDOWN) return;
  }
  switch(event.type) {
  case Event::MOUSEMOTION: {
    if(dragging) {
      viewport -= event.mousemove;
      constrainViewportPosition(true);
      viewportUpdated();
      setPanningCursor();
      setDirty();
      break;
    }
    else {
      scrollCorrection = Vector2(0,0);
    }

    mouseScrollState = SCROLL_NONE;
    if(event.mousepos.x < scrollBorder)
      mouseScrollState |= SCROLL_LEFT;
    else if(event.mousepos.x > getWidth() - scrollBorder)
      mouseScrollState |= SCROLL_RIGHT;

    if(event.mousepos.y < scrollBorder)
      mouseScrollState |= SCROLL_UP;
    else if(event.mousepos.y > getHeight() - scrollBorder)
      mouseScrollState |= SCROLL_DOWN;

    if(!event.inside) {
      mouseInGameView = false;
      break;
    }
    mouseInGameView = true;

    MapPoint tile = getTile(event.mousepos);
    if(!roadDragging && leftButtonDown && cursorSize == 1
      && getUserOperation()->action != UserOperation::ACTION_EVACUATE
    ) {
      roadDragging = true;
      startRoad = tile;
      areaBulldoze = (SDL_GetModState() & SDL_KMOD_CTRL);
    }

    if(roadDragging && cursorSize != 1) {
      roadDragging = false;
      areaBulldoze = false;
      ctrDrag = false;
    }
    // bulldoze at once while still dragging

    if(roadDragging && !areaBulldoze
      && getUserOperation()->action == UserOperation::ACTION_BULLDOZE
      && tile != startRoad
    ) {
      game->executeUserOperation(startRoad);
      startRoad = tile;
    }

    if(tileUnderMouse != tile) {
      tileUnderMouse = tile;
      setDirty();
      //update mps target
      if(getUserOperation()->action == UserOperation::ACTION_EVACUATE)
        game->getMpsMap().setTile(
          game->getWorld().map.is_visible(tile)
          ? game->getWorld().map(tile)
          : nullptr
        );
    }
  } break;
  case Event::MOUSEBUTTONDOWN: {
    if(!event.inside);
    else if(event.mousebutton == SDL_BUTTON_MIDDLE) {
      dragging = true;
      ctrDrag = false;
      mouseScrollState = SCROLL_NONE;
    }
    else if(event.mousebutton == SDL_BUTTON_LEFT) {
      roadDragging = false;
      areaBulldoze = false;
      ctrDrag = false;
      leftButtonDown = true;
    }
  } break;
  case Event::MOUSEBUTTONUP: {
    if(event.mousebutton == SDL_BUTTON_MIDDLE) {
      if(dragging) {
        dragging = false;
        setDefaultCursor();
        break;
      }
      dragging = false;
    }
    if(event.mousebutton == SDL_BUTTON_LEFT) {
      if(roadDragging && event.inside) {
        MapPoint endRoad = getTile(event.mousepos);
        roadDragging = false;
        areaBulldoze = false;
        leftButtonDown = false;
        if(cursorSize != 1)
          //roadDragging was aborted with Escape
          break;
        if(blockingDialogIsOpen)
          break;

        //build last tile first to play the sound
        Message::ptr dummyMsg;
        // check if allowed to avoid many dialogs for bulk ops
        if(getUserOperation()->isAllowedHere(
          game->getWorld(), endRoad, dummyMsg)
        ) {
          game->executeUserOperation(endRoad);
        }
        MapPoint currentTile = startRoad;
        int stepx = ( startRoad.x > endRoad.x ) ? -1 : 1;
        int stepy = ( startRoad.y > endRoad.y ) ? -1 : 1;
        if(getUserOperation()->action ==
          UserOperation::ACTION_BULLDOZE
        ) {
          for(currentTile.x = startRoad.x;
            currentTile.x != endRoad.x + stepx;
            currentTile.x += stepx
          )
          for(currentTile.y = startRoad.y;
            currentTile.y != endRoad.y + stepy;
            currentTile.y += stepy
          ) {
            if(getUserOperation()->isAllowedHere(
              game->getWorld(), currentTile, dummyMsg)
            ) {
              game->executeUserOperation(currentTile);
            }
          }
        }
        else if(getUserOperation()->action ==
          UserOperation::ACTION_BUILD
        ) {
          int* v1 = ctrDrag ? &currentTile.y : &currentTile.x;
          int* v2 = ctrDrag ? &currentTile.x : &currentTile.y;
          int* l1 = ctrDrag ? &endRoad.y : &endRoad.x;
          int* l2 = ctrDrag ? &endRoad.x : &endRoad.y;
          int* s1 = ctrDrag ? &stepy : &stepx;
          int* s2 = ctrDrag ? &stepx : &stepy;

          while(*v1 != *l1) {
            if(getUserOperation()->isAllowedHere(
              game->getWorld(), currentTile, dummyMsg)
            ) {
              game->executeUserOperation(currentTile);
            }
            *v1 += *s1;
          }
          while(*v2 != *l2) {
            if(getUserOperation()->isAllowedHere(
              game->getWorld(), currentTile, dummyMsg)
            ) {
              game->executeUserOperation(currentTile);
            }
            *v2 += *s2;
          }
        }
        break;
      }
      roadDragging = false;
      ctrDrag = false;
      areaBulldoze = false;
      leftButtonDown = false;
    }
    if(!event.inside)
      break;

    if(event.mousebutton == SDL_BUTTON_LEFT) {
      if(!blockingDialogIsOpen) {
        game->executeUserOperation(getTile(event.mousepos));
      }
    }
    else if(event.mousebutton == SDL_BUTTON_RIGHT) {
      // show info on the clicked thing
      MapPoint point = getTile(event.mousepos);
      if(!inCity(point)) break;
      game->getMpsMap().query(game->getWorld().map(point));
      game->getMiniMap().switchView("MapMPS");
    }
  } break;
  case Event::MOUSEWHEEL: {
    if(!event.inside || event.scrolly == 0)
      break;
    if(event.scrolly > 0)
      zoomMouse(sqrt(2.f), event.mousepos);
    else
      zoomMouse(sqrt(0.5), event.mousepos);
  } break;
  case Event::WINDOWLEAVE: {
    mouseInGameView = false;
    mouseScrollState = SCROLL_NONE;
  } break;
  case Event::WINDOWENTER:
    break;
  case Event::KEYDOWN: {
    switch(event.scancode) {
    case SDL_SCANCODE_LCTRL:
    case SDL_SCANCODE_RCTRL:
      if(roadDragging)
        ctrDrag = !ctrDrag;
      break;
    case SDL_SCANCODE_KP_8:
    case SDL_SCANCODE_UP:
    case SDL_SCANCODE_W:
      keyScrollState |= SCROLL_UP;
      break;
    case SDL_SCANCODE_KP_2:
    case SDL_SCANCODE_DOWN:
    case SDL_SCANCODE_S:
      keyScrollState |= SCROLL_DOWN;
      break;
    case SDL_SCANCODE_KP_4:
    case SDL_SCANCODE_LEFT:
    case SDL_SCANCODE_A:
      keyScrollState |= SCROLL_LEFT;
      break;
    case SDL_SCANCODE_KP_6:
    case SDL_SCANCODE_RIGHT:
    case SDL_SCANCODE_D:
      keyScrollState |= SCROLL_RIGHT;
      break;
    case SDL_SCANCODE_KP_7:
      keyScrollState |= SCROLL_UP_LEFT;
      break;
    case SDL_SCANCODE_KP_9:
      keyScrollState |= SCROLL_UP_RIGHT;
      break;
    case SDL_SCANCODE_KP_1:
      keyScrollState |= SCROLL_DOWN_LEFT;
      break;
    case SDL_SCANCODE_KP_3:
      keyScrollState |= SCROLL_DOWN_RIGHT;
      break;
    case SDL_SCANCODE_LSHIFT:
      keyScrollState |= SCROLL_LSHIFT;
      break;
    case SDL_SCANCODE_RSHIFT:
      keyScrollState |= SCROLL_RSHIFT;
      break;
    case SDL_SCANCODE_N:
      getMiniMap()->scrollPageDown(true);
      break;
    case SDL_SCANCODE_M:
      getMiniMap()->scrollPageDown(false);
      break;
    }
  } break;
  case Event::KEYUP: {
    switch(event.scancode) {
    case SDL_SCANCODE_G: {
      MpsMap& mps = game->getMpsMap();
      if(mps.page == MpsMap::Page::GROUND)
        mps.page = MpsMap::Page::INVENTORY;
      else
        mps.page = MpsMap::Page::GROUND;
      mps.refresh();
      game->getMiniMap().switchView("MapMPS");
      break;
    }
    case SDL_SCANCODE_H: {
      hideHigh = !hideHigh;
      updateBuildingViewIndicator();
      setDirty();
      setMapDirty();
    } break;
    case SDL_SCANCODE_V: {
      mapOverlay = (mapOverlay+1)%(overlayMAX+1);
      setDirty();
      setMapDirty();
    } break;
    //Zoom
    case SDL_SCANCODE_KP_PLUS:
      zoomIn();
      break;
    case SDL_SCANCODE_KP_MINUS:
      zoomOut();
      break;
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_RETURN2:
      resetZoom();
      break;
    //Scroll
    case SDL_SCANCODE_KP_8:
    case SDL_SCANCODE_UP:
    case SDL_SCANCODE_W:
      keyScrollState &= ~SCROLL_UP;
      break;
    case SDL_SCANCODE_KP_2:
    case SDL_SCANCODE_DOWN:
    case SDL_SCANCODE_S:
      keyScrollState &= ~SCROLL_DOWN;
      break;
    case SDL_SCANCODE_KP_4:
    case SDL_SCANCODE_LEFT:
    case SDL_SCANCODE_A:
      keyScrollState &= ~SCROLL_LEFT;
      break;
    case SDL_SCANCODE_KP_6:
    case SDL_SCANCODE_RIGHT:
    case SDL_SCANCODE_D:
      keyScrollState &= ~SCROLL_RIGHT;
      break;
    case SDL_SCANCODE_KP_7:
      keyScrollState &= ~SCROLL_UP_LEFT;
      break;
    case SDL_SCANCODE_KP_9:
      keyScrollState &= ~SCROLL_UP_RIGHT;
      break;
    case SDL_SCANCODE_KP_1:
      keyScrollState &= ~SCROLL_DOWN_LEFT;
      break;
    case SDL_SCANCODE_KP_3:
      keyScrollState &= ~SCROLL_DOWN_RIGHT;
      break;
    case SDL_SCANCODE_LSHIFT:
      keyScrollState &= ~SCROLL_LSHIFT;
      break;
    case SDL_SCANCODE_RSHIFT:
      keyScrollState &= ~SCROLL_RSHIFT;
      break;
    case SDL_SCANCODE_KP_5: {
      show(MapPoint(getWorld().map.len() / 2, getWorld().map.len() / 2));
      setDirty();
    } break;
    }
  } break;
  case Event::UPDATE: {
    scroll(event.elapsedTime);
  } break;
  default:
    break;
  }
}

void GameView::setPanningCursor() {
    desktop->setSystemCursor(this, SDL_SYSTEM_CURSOR_MOVE);
}

void GameView::setDefaultCursor() {
    desktop->tryClearCursor(this);
}

/*
 * Parent tells us to change size.
 */
void GameView::resize(float newwidth , float newheight )
{
    width = newwidth;
    height = newheight;
    if(width < 0) width = 0;
    if(height < 0) height = 0;
    viewportUpdated();
    setDirty();
}

/*
 *  We should draw the whole City again.
 */
void GameView::viewportUpdated()
{
    if( !getMiniMap() ){ //initialization not completed
        return;
    }

    static MapPoint oldCenter = this->getCenter();
    static float oldZoom = zoom;

    MapPoint newCenter = this->getCenter();

    if ( (oldCenter != newCenter) || (oldZoom != zoom) ) {
      //Tell Minimap about new Corners
      Vector2 viewport2 = viewport + getSize();
      getMiniMap()->setGameViewCorners(
        Vector2(
          viewport.y / tileHeight + viewport.x / tileWidth,
          viewport.y / tileHeight - viewport.x / tileWidth
        ),
        Vector2(
          viewport2.y / tileHeight + viewport2.x / tileWidth,
          viewport2.y / tileHeight - viewport2.x / tileWidth
        )
      );

      oldCenter = newCenter;
      oldZoom = zoom;
    }

    refreshMap = true;
}

/*
 * Find point on Screen, where lower right corner of tile
 * is placed.
 */
Vector2 GameView::getScreenPoint(MapPoint mp)
{
    Vector2 point;
    point.x = (mp.x - mp.y) * ( tileWidth / 2 );
    point.y = (mp.x + mp.y) * ( tileHeight / 2 );

    //we want the lower right corner
    point.y += tileHeight;

    if ((showTerrainHeight) && (inCity(mp))){
        // shift the tile upward to show altitude
        point.y -= (float) ( (getWorld().map(mp)->ground.altitude) * scale3d) * zoom  / (float) getWorld().map.alt_step ;
    }

    //on Screen
    point -= viewport;

    return point;
}

/*
 * Find Tile at point on viewport
 */
MapPoint GameView::getTile(const Vector2& p)
{
    MapPoint tile;
    // Map Point to virtual Screen
    Vector2 point = p + viewport;
    tile.x = (int) floorf(point.y / tileHeight + point.x / tileWidth);
    tile.y = (int) floorf(point.y / tileHeight - point.x / tileWidth);

    return tile;
}

/*
 * Draw a filled Diamond inside given Rectangle
 */
void GameView::fillDiamond( Painter& painter, const Rect2D& rect )
{
    Vector2 points[ 4 ];
    points[ 0 ].x = rect.p1.x + ( rect.getWidth() / 2 );
    points[ 0 ].y = rect.p1.y;
    points[ 1 ].x = rect.p1.x;
    points[ 1 ].y = rect.p1.y + ( rect.getHeight() / 2 );
    points[ 2 ].x = rect.p1.x + ( rect.getWidth() / 2 );
    points[ 2 ].y = rect.p2.y;
    points[ 3 ].x = rect.p2.x;
    points[ 3 ].y = rect.p1.y + ( rect.getHeight() / 2 );
    painter.fillPolygon( 4, points );
}

/*
 * Draw a outlined Diamond inside given Rectangle
 */
void GameView::drawDiamond( Painter& painter, const Rect2D& rect )
{
    if( !buttonsConnected ) connectButtons();//has to be done here,
                    // because now everything else is initialized.
    Vector2 points[ 4 ];
    points[ 0 ].x = rect.p1.x + ( rect.getWidth() / 2 );
    points[ 0 ].y = rect.p1.y;
    points[ 1 ].x = rect.p1.x;
    points[ 1 ].y = rect.p1.y + ( rect.getHeight() / 2 );
    points[ 2 ].x = rect.p1.x + ( rect.getWidth() / 2 );
    points[ 2 ].y = rect.p2.y;
    points[ 3 ].x = rect.p2.x;
    points[ 3 ].y = rect.p1.y + ( rect.getHeight() / 2 );
    painter.drawPolygon( 4, points );
}

/*
 * Check if Tile is in City
 * in oldgui you can edit (1,1) to (98,98) with world.len() 100
 * i.e. there is a hidden border of green tiles arround the city
 */
bool GameView::inCity( MapPoint tile )
{
    return getWorld().map.is_visible(tile);
}

/*
 * Draw MiniMapOverlay for tile.
 */
void GameView::drawOverlay(Painter& painter, const MapPoint &tile){
    Color black;
    black.parse("black");
    Color miniMapColor;

    Vector2 tileOnScreenPoint = getScreenPoint(tile);
    Rect2D tilerect( 0, 0, tileWidth, tileHeight );
    tileOnScreenPoint.x = tileOnScreenPoint.x - ( tileWidth / 2);
    tileOnScreenPoint.y -= tileHeight;
    tilerect.move( tileOnScreenPoint );
    //Outside of the Map gets Black overlay
    if( !inCity( tile ) ) {
            painter.setFillColor( black );
    } else {
        miniMapColor = getMiniMap()->getColor(*getWorld().map(tile));
        if( mapOverlay == overlayOn ){
            miniMapColor.a = 200;  //Transparent
        }
        painter.setFillColor( miniMapColor );
    }
    fillDiamond( painter, tilerect );
}

/*
 * If the current Tile is Part of a Building, return the
 * Coordinates of the tile that contains the real informations.
 */
MapPoint GameView::realTile(MapPoint point) {
  if(!inCity(point))
    return point;
  else if(getWorld().map(point)->reportingConstruction)
    return getWorld().map(point)->reportingConstruction->point;
  else
    return point;
}

void GameView::fetchTextures() {
  std::map<std::string, ResourceGroup*>::iterator it;
  for(it= ResourceGroup::resMap.begin(); it != ResourceGroup::resMap.end(); ++it) {
    for(size_t i = 0; i < it->second->graphicsInfoVector.size(); ++i) {
      auto& gfx = it->second->graphicsInfoVector[i];
      if(!gfx.texture && gfx.image) {
        gfx.texture = texture_manager->create(gfx.image);
        if(gfx.texture) { //Image was erased by texture_manager->create.
          gfx.texture->setScaleMode(Texture::ScaleMode::NEAREST);
          gfx.image = 0;
        }
        --remaining_images;
      }
    }
  }
}



void GameView::drawTexture(Painter& painter, const MapPoint &tile, GraphicsInfo *graphicsInfo)
{
    Rect2D tilerect( 0, 0, tileWidth, tileHeight );
    Vector2 tileOnScreenPoint = getScreenPoint( tile );
    // Test if we have to convert Preloaded Image to Texture
    if( !graphicsInfo->texture ) //&& !economyGraph_open
    {
        if(graphicsInfo->image)
        {
            graphicsInfo->texture = texture_manager->create( graphicsInfo->image );
            if(graphicsInfo->texture) { //Image was erased by texture_manager->create.
              graphicsInfo->texture->setScaleMode(Texture::ScaleMode::NEAREST);
              graphicsInfo->image = 0;
            }
            --remaining_images;
        }
    }
    if (graphicsInfo->texture)
    {
        tileOnScreenPoint.x -= graphicsInfo->x * zoom;
        tileOnScreenPoint.y -= graphicsInfo->y * zoom;
        tilerect.move( tileOnScreenPoint );
        tilerect.setSize(graphicsInfo->texture->getWidth() * zoom,
            graphicsInfo->texture->getHeight() * zoom);
        // Light sprites in the map render pass instead of painting a veil over
        // the completed scene. Terrain, powered buildings and UI stay distinct.
        const auto& e = getWorld().evolution;
        const int dark = (graphicsInfo->nightKey.rfind("Fire:",0)==0 || graphicsInfo->nightKey.rfind("FireWasteLand:",0)==0)?0:(e.darkness()/25)*25;
        bool powered = false;
        if(inCity(tile)) {
          auto* c = getWorld().map(tile)->reportingConstruction;
          if(c) powered = GROUP_IS_RESIDENCE(c->constructionGroup->group)
            ? bool(c->flags & FLAG_HAD_POWER) : c->commodityCount[STUFF_LOVOLT] > 0;
        }
        const int cloud = e.currentWeather() ? 10 : 0;
        const int strength = powered ? 110 : 148;
        graphicsInfo->texture->setLighting(255 - dark*strength/1000 - cloud,
          255 - dark*(strength-12)/1000 - cloud,
          255 - dark*(strength-35)/1000 - cloud);
        painter.drawStretchTexture(graphicsInfo->texture, tilerect);
        graphicsInfo->texture->setLighting(255,255,255);
        bool occupied=true;
        if(inCity(tile)) { auto* c=getWorld().map(tile)->reportingConstruction; if(c){int group=c->constructionGroup->group;if(GROUP_IS_RESIDENCE(group))occupied=static_cast<Residence*>(c)->local_population>0;else if(group==GROUP_HEALTH)occupied=static_cast<HealthCentre*>(c)->active;else if(group==GROUP_FIRESTATION)occupied=static_cast<FireStation*>(c)->active;else if(group==GROUP_POLICE)occupied=getWorld().civic.stationReady(*c,CivicServices::POLICE);} }
        drawNightLayers(painter,graphicsInfo->nightKey,tilerect,occupied,powered);
    }
}


void GameView::drawTile(Painter& painter, const MapPoint &tile)
{

    //is Tile in City? If not draw Blank
    if( ! inCity( tile ) )
    {
        drawTexture(painter, tile, &blankGraphicsInfo);
        return;
    }

    //Texture* texture = 0;
    MapPoint upperLeft = realTile(tile);

    ConstructionGroup *cstgrp =
      getWorld().map(upperLeft)->getTopConstructionGroup();
    ResourceGroup *resgrp;
    unsigned short size = cstgrp->size;

    //Attention map is rotated for displaying
    if(tile.n(size-1) == upperLeft) //Signs are tested
    {
        resgrp = getWorld().map(upperLeft)->getTileResourceGroup();
        //adjust OnScreenPoint of big Tiles
        MapPoint lowerRightTile(tile.e(size-1));
        unsigned short textureType = getWorld().map(upperLeft)->getTopType();

        // if we hide high buildings, hide trees as well
        if (hideHigh && (cstgrp == &treeConstructionGroup
         || cstgrp == &tree2ConstructionGroup
         || cstgrp == &tree3ConstructionGroup ))
        {
            cstgrp = &bareConstructionGroup;
            resgrp = ResourceGroup::resMap["Green"];
        }
        GraphicsInfo *graphicsInfo = 0;
        //draw visible tiles underneath constructions
        if(Construction *cst = getWorld().map(upperLeft)->reportingConstruction;
          !cst || (cst->flags & FLAG_TRANSPARENT))
        {
            if (resgrp->images_loaded)
            {
                size_t s = resgrp->graphicsInfoVector.size();
                if (s)
                {
                    graphicsInfo = &resgrp->graphicsInfoVector
                        [ getWorld().map(upperLeft)->type  % s];
                    drawTexture(painter, lowerRightTile, graphicsInfo);
                }
            }
        }
        bool draw_colored_site = true;

        if(!hideHigh || (size==1 && (GROUP_IS_BARE(cstgrp->group) || cstgrp->group==GROUP_WATER || cstgrp->group==GROUP_POWER_LINE || cstgrp->group==GROUP_FIRE || cstgrp->group==GROUP_ROAD || cstgrp->group==GROUP_ROAD_BRIDGE || cstgrp->group==GROUP_TRACK || cstgrp->group==GROUP_TRACK_BRIDGE || cstgrp->group==GROUP_RAIL || cstgrp->group==GROUP_RAIL_BRIDGE)))
        {
            draw_colored_site = false;
            if (getWorld().map(upperLeft)->framesptr)
            {
                for(std::list<ExtraFrame>::iterator frit = getWorld().map(upperLeft)->framesptr->begin();
                    frit != getWorld().map(upperLeft)->framesptr->end(); std::advance(frit, 1))
                {
                    if(frit->resourceGroup && frit->resourceGroup->images_loaded)
                    {
                        size_t s2 = frit->resourceGroup->graphicsInfoVector.size();
                        if((frit->frame >= 0) && s2)
                        {
                            graphicsInfo = &frit->resourceGroup->graphicsInfoVector[ frit->frame % s2 ]; //needed
                            int old_x = graphicsInfo->x;
                            int old_y = graphicsInfo->y;
                            float ms=(frit->resourceGroup==ResourceGroup::resMap["GraySmoke"]||frit->resourceGroup==ResourceGroup::resMap["BlackSmoke"])?0.8f:1.0f;
                            graphicsInfo->x = old_x - int(std::lround(frit->move_x*ms));
                            graphicsInfo->y = old_y - int(std::lround(frit->move_y*ms));
                            if( frit->resourceGroup->is_vehicle)
                            {
                                //only draw vehicles on top of...
                                switch (cstgrp->group)
                                {
                                    case GROUP_BARE:
                                    case GROUP_DESERT:
                                    case GROUP_WATER:
                                    case GROUP_TRACK:
                                    case GROUP_TRACK_BRIDGE:
                                    case GROUP_ROAD:
                                    case GROUP_ROAD_BRIDGE:
                                    case GROUP_RAIL:
                                    case GROUP_RAIL_BRIDGE:
                                    case GROUP_ORGANIC_FARM:
                                    case GROUP_WATERWELL:
                                    case GROUP_SHANTY:
                                    case GROUP_CRICKET:
                                    case GROUP_SUBSTATION:
                                    //add anything else with low northen corner
                                        drawTexture(painter, upperLeft, graphicsInfo);
                                    break;
                                    default:
                                    break;
                                }


                            }
                            else
                            {   drawTexture(painter, lowerRightTile, graphicsInfo);}
                            graphicsInfo->x = old_x;
                            graphicsInfo->y = old_y;
                        }
                    }
                    else
                    {   draw_colored_site = true;}
                }
            }
        }

        if(draw_colored_site)
        {
            Rect2D tilerect( 0, 0, size * tileWidth, size * tileHeight );
            Vector2 tileOnScreenPoint = getScreenPoint( lowerRightTile );
            tileOnScreenPoint.x =  tileOnScreenPoint.x - ( tileWidth*size / 2);
            tileOnScreenPoint.y -= tileHeight*size;
            tilerect.move( tileOnScreenPoint );
            painter.setFillColor(getMiniMap()->getColorNormal(*getWorld().map(tile)));
            fillDiamond( painter, tilerect );
        }
        if(mapOverlay != overlayOnly && (cstgrp->group == GROUP_ROAD || cstgrp->group == GROUP_TRACK
          || cstgrp->group == GROUP_ROAD_BRIDGE || cstgrp->group == GROUP_TRACK_BRIDGE))
          drawStreetActors(painter, tile);
        if(mapOverlay != overlayOnly && (cstgrp->group==GROUP_ROAD || cstgrp->group==GROUP_ROAD_BRIDGE || cstgrp->group==GROUP_RAIL || cstgrp->group==GROUP_RAIL_BRIDGE)) drawTransit(painter,0,tile);
        if(mapOverlay != overlayOnly) drawEmergencyActors(painter,tile);
        if(!hideHigh && mapOverlay!=overlayOnly) {
          auto* parent=getWorld().map(upperLeft)->reportingConstruction;
          if(parent){auto stock=buildingStockVisual(*parent);if(stock.kind>=0&&stock.level>0){const char* kinds[]={"food","goods","ore","coal"};auto tx=texture_manager->load(std::string("images/activity/")+kinds[stock.kind]+"-"+std::to_string(stock.level)+".png");auto p=getScreenPoint(lowerRightTile)-Vector2(tileWidth*.60f,tileHeight*.48f);int dark=getWorld().evolution.darkness();tx->setLighting(255-dark*148/1000,255-dark*136/1000,255-dark*113/1000);painter.drawStretchTexture(tx,Rect2D(p.x-25*zoom,p.y-47*zoom,p.x+25*zoom,p.y+3*zoom));tx->setLighting(255,255,255);}}
        }
        // Static details follow their building visibility.
        if(!hideHigh && mapOverlay != overlayOnly &&
          (cstgrp->group==GROUP_ORGANIC_FARM || cstgrp->group==GROUP_WAREHOUSE || cstgrp->group==GROUP_MARKET)) {
          bool motor=TrafficEra::motorVehicle(getWorld().tech_level,MAX_TECH_LEVEL,unsigned(upperLeft.x*131+upperLeft.y*17));
          const char* name=cstgrp->group==GROUP_ORGANIC_FARM?"horse-cart":cstgrp->group==GROUP_WAREHOUSE?(motor?"truck":"horse-cart"):"horse";
          auto texture=texture_manager->load(std::string("images/transit/")+name+".png");
          auto p=getScreenPoint(lowerRightTile)-Vector2(tileWidth*.2f,tileHeight*.65f);
          float width=texture->getWidth()*zoom,height=texture->getHeight()*zoom;
          Rect2D detailRect(p.x-width/2,p.y-height,p.x+width/2,p.y);
          int dark=getWorld().evolution.darkness();texture->setLighting(255-dark*110/1000,255-dark*95/1000,255-dark*65/1000);painter.drawStretchTexture(texture,detailRect);texture->setLighting(255,255,255);
          drawNightLayers(painter,std::string("Traffic:")+name+":0",detailRect,true,true);
        }
        //last draw suspended power cables on top
        //only works for size == 1
        if (getWorld().map(upperLeft)->flags & (FLAG_POWER_CABLES_0 | FLAG_POWER_CABLES_90))
        {
            resgrp = ResourceGroup::resMap["PowerLine"];
            if(resgrp->images_loaded)
            {
                if (getWorld().map(upperLeft)->flags & FLAG_POWER_CABLES_0)
                {   drawTexture(painter, upperLeft, &resgrp->graphicsInfoVector[23]);}
                if (getWorld().map(upperLeft)->flags & FLAG_POWER_CABLES_90)
                {   drawTexture(painter, upperLeft, &resgrp->graphicsInfoVector[22]);}
            }
        }
    }


}

/*
 * Mark a tile with current cursor
 */
void GameView::markTile(Painter& painter, MapPoint point) {
    Vector2 tileOnScreenPoint = getScreenPoint(point);
    {
      MapPoint upperLeft = realTile(point);
      Construction *cst;
      if(game->getMpsMap().tile
        && upperLeft == realTile(game->getMpsMap().tile->point)
        && getUserOperation()->action == UserOperation::ACTION_QUERY
        && (cst = getWorld().map(point)->reportingConstruction)
      ) {
        ConstructionGroup *constructionGroup = cst->constructionGroup;
        int range = constructionGroup->range;
        int edgelen = 2 * range + constructionGroup->size;
        painter.setFillColor(Color(0, 255, 0, 64));
        Rect2D rangerect(0, 0, tileWidth * edgelen, tileHeight * edgelen);
        Vector2 screenPoint = getScreenPoint(upperLeft);
        screenPoint.x -= tileWidth * (0.5*edgelen);
        screenPoint.y -= tileHeight * (range + 1);
        rangerect.move(screenPoint);
        fillDiamond(painter, rangerect);
      }
    }

    if( getUserOperation()->action == UserOperation::ACTION_QUERY) //  cursorSize == 0
    {
        Color alphawhite( 255, 255, 255, 128 );
        painter.setLineColor( alphawhite );
        Rect2D tilerect( 0, 0, tileWidth, tileHeight );
        tileOnScreenPoint.x -= ( tileWidth / 2);
        tileOnScreenPoint.y -= tileHeight;
        tilerect.move( tileOnScreenPoint );
        drawDiamond( painter, tilerect );
    }
    else
    {
        Color alphablue( 0, 0, 255, 128 );
        Color alphared( 255, 0, 0, 128 );
        Message::ptr tmpMsg;
        if(getUserOperation()->isAllowedHere(getWorld(), point, tmpMsg))
          painter.setFillColor(alphablue);
        else
          painter.setFillColor(alphared);

        Rect2D tilerect( 0, 0, tileWidth * cursorSize, tileHeight * cursorSize );
        tileOnScreenPoint.x -= (tileWidth * cursorSize / 2);
        tileOnScreenPoint.y -= tileHeight;
        tilerect.move( tileOnScreenPoint );
        fillDiamond( painter, tilerect );

        if(getUserOperation()->action == UserOperation::ACTION_BUILD)
        {
            // Draw range for selected_building
            int range = getUserOperation()->constructionGroup->range;
            if (range > 0 )
            {
                int edgelen = 2 * range + getUserOperation()->constructionGroup->size ;
                painter.setFillColor( Color( 0, 0, 128, 64 ) );
                Rect2D rangerect( 0,0,
                                  tileWidth  * ( edgelen) ,
                                  tileHeight * ( edgelen) );
                Vector2 screenPoint = getScreenPoint(point);
                screenPoint.x -= tileWidth  * ( 0.5*edgelen );
                screenPoint.y -= tileHeight * ( range + 1 );
                rangerect.move( screenPoint );
                fillDiamond( painter, rangerect );
            }//endif range > 0
        }
    }//endelse ACTION_QUERY
}

/*
 *  Paint an isometric View of the City in the component.
 */
void GameView::draw(Painter& painter)
{
    updateStreetActors();
    int frame=getWorld().evolution.visualTime/80;
    if(!getWorld().transit.routes.empty()&&frame!=lastTransitFrame){refreshMap=true;lastTransitFrame=frame;}
    const auto& sky = getWorld().evolution;
    const int lightLevel = sky.darkness()/25 + sky.currentWeather()*100;
    if(lightLevel != lastLightLevel) { refreshMap = true; lastLightLevel = lightLevel; }
    // Constraining the position here shouldn't be necessary if other parts of the code are correct.
    // if( constrainViewportPosition(true) ) {
    //     // Returning causes the display to lag. I'm not sure why it's needed anyway.
    //     // return;
    // }

    //The Corners of The Screen
    Vector2 upperLeft( 0, 0);
    Vector2 upperRight( getWidth(), 0 );
    Vector2 lowerLeft( 0, getHeight() );

    if (showTerrainHeight)
    {
        // printf("h = %f,     z = %f \n ", getHeight(), zoom);
        // getHeight = size in pixel of the screen (eg 1024x768)
        Vector2 lowerLeft( 0, getHeight() * ( 1 + getHeight() * zoom / (float)scale3d ));
    }

    //Find visible Tiles
    MapPoint upperLeftTile  = getTile( upperLeft );
    MapPoint upperRightTile = getTile( upperRight );
    MapPoint lowerLeftTile  = getTile( lowerLeft );

    //draw Background
    Color black;
    black.parse("black");
    painter.setFillColor(black);
    painter.clear();

    //draw Tiles
    MapPoint currentTile;
    //Draw some extra tiles depending on the maximal size of a building.
    int extratiles = 7;
    upperLeftTile.x -= extratiles;
    upperRightTile.y -= extratiles;
    upperRightTile.x += extratiles;
    lowerLeftTile.y +=  extratiles;

    if(!mapTexture || refreshMap) {
      if(!mapTexture
        || mapTexture->getWidth() != (int)getWidth()
        || mapTexture->getHeight() != (int)getHeight()
      ) {
        mapTexture = painter.createTargetTexture(getWidth(), getHeight());
      }

      painter.pushRenderTarget(mapTexture.get());
      painter.setFillColor(Color(0,0,0));
      painter.clear();

      if(mapOverlay != overlayOnly) {
        for(int k = 0; k <= 2 * (lowerLeftTile.y - upperLeftTile.y); k++)
        for(int i = 0; i <= upperRightTile.x - upperLeftTile.x; i++) {
          currentTile.x = upperLeftTile.x + i + k / 2 + k % 2;
          currentTile.y = upperLeftTile.y - i + k / 2;
          drawTile(painter, currentTile);
        }
      }
      if(mapOverlay != overlayNone) {
        for(int k = 0; k <= 2 * (lowerLeftTile.y - upperLeftTile.y); k++)
        for(int i = 0; i <= upperRightTile.x - upperLeftTile.x; i++) {
          currentTile.x = upperLeftTile.x + i + k / 2 + k % 2;
          currentTile.y = upperLeftTile.y - i + k / 2;
          drawOverlay(painter, currentTile);
        }
      }
      painter.popRenderTarget();
      refreshMap = false;
    }
    painter.drawTexture(mapTexture.get(), Vector2(0,0));
    if(mapOverlay != overlayOnly) {drawTransit(painter,1);drawCityLife(painter);}

    int cost = 0;
    //display commodities continously
    if(getUserOperation()->action == UserOperation::ACTION_EVACUATE) {
      game->getMpsMap().refresh();
      game->getMiniMap().switchView("MapMPS");
    }
    drawDistricts(painter);
    //Mark Tile under Mouse // TODO: handle showTerrainHeight
    if(mouseInGameView && !blockingDialogIsOpen) {
        MapPoint lastRazed( -1,-1 );
        int tiles = 0;
        if( roadDragging && ( cursorSize == 1 ) &&
        (getUserOperation()->action == UserOperation::ACTION_BUILD || getUserOperation()->action == UserOperation::ACTION_BULLDOZE))
        {
            //use same method to find all Tiles as in GameView::event(const Event& event)
            int stepx = ( startRoad.x > tileUnderMouse.x ) ? -1 : 1;
            int stepy = ( startRoad.y > tileUnderMouse.y ) ? -1 : 1;
            currentTile = startRoad;

            if (getUserOperation()->action == UserOperation::ACTION_BULLDOZE)
            {
                for (;currentTile.x != tileUnderMouse.x + stepx; currentTile.x += stepx) {
                    for (currentTile.y = startRoad.y; currentTile.y != tileUnderMouse.y + stepy; currentTile.y += stepy) {
                        markTile( painter, currentTile );
                        if( realTile( currentTile ) != lastRazed ){
                            cost += bulldozeCost( currentTile );
                            lastRazed = realTile( currentTile );
                        }
                        tiles++;
                    }
                }
            }
            else if (getUserOperation()->action == UserOperation::ACTION_BUILD)
            {
                int* v1 = ctrDrag ? &currentTile.y :&currentTile.x;
                int* v2 = ctrDrag ? &currentTile.x :&currentTile.y;
                int* l1 = ctrDrag ? &tileUnderMouse.y :&tileUnderMouse.x;
                int* l2 = ctrDrag ? &tileUnderMouse.x :&tileUnderMouse.y;
                int* s1 = ctrDrag ? &stepy: &stepx;
                int* s2 = ctrDrag ? &stepx: &stepy;

                while( *v1 != *l1)
                {
                    markTile( painter, currentTile );
                    cost += buildCost( currentTile );
                    tiles++;
                    *v1 += *s1;
                }
                while( *v2 != *l2 + *s2 )
                {
                    markTile( painter, currentTile );
                    cost += buildCost( currentTile );
                    tiles++;
                    *v2 += *s2;
                }

            }
        }
        else
        {
            markTile( painter, tileUnderMouse );
            tiles++;
            if( (getUserOperation()->action == UserOperation::ACTION_BULLDOZE ) && realTile( currentTile ) != lastRazed ) {
                    cost += bulldozeCost( tileUnderMouse );
            } else {
                cost += buildCost( tileUnderMouse );
            }
        }
        std::stringstream prize;
        if( getUserOperation()->action == UserOperation::ACTION_BULLDOZE ){
            if( roadDragging ){
                prize << _("Estimated Bulldoze Cost: ");
            } else {
                prize << _("Bulldoze Cost: ");
            }
            if( cost > 0 ) {
                prize << cost << _("$");
            } else {
                prize << _("n/a");
            }
            printStatusMessage( prize.str() );
        }
        else if( getUserOperation()->action == UserOperation::ACTION_BUILD)
        {
            std::string buildingName =  getUserOperation()->constructionGroup->name;
            prize << _(buildingName);
            prize << _(": Cost to build ");
            if( cost > 0 ) {
                prize << cost << _("$");
        } else {
                prize << _("n/a");
            }
            printStatusMessage( prize.str() );
        } else {
           showToolInfo( tiles );
        }
        {
          auto& ev = getWorld().evolution;
          if(districtMode && districtDrag) {
            printStatusMessage(std::string(districtType == 0 ? "Mayor's work zone: " : std::string("New ") + CityEvolution::districtTypeName(districtType) + " district: ")
              + std::to_string(std::abs(districtEnd.x - districtStart.x) + 1) + "x" + std::to_string(std::abs(districtEnd.y - districtStart.y) + 1)
              + " tiles. Release the mouse to create it.");
          } else if(districtMode) {
            printStatusMessage(districtType == 0 ? std::string("Drag a rectangle for the mayor's work zone. Esc cancels.")
              : std::string("Drag a rectangle for the new ") + CityEvolution::districtTypeName(districtType) + " district. Esc cancels.");
          } else if(getUserOperation()->action == UserOperation::ACTION_QUERY) {
            const int di = ev.districtAt(tileUnderMouse.x, tileUnderMouse.y);
            if(di >= 0) printStatusMessage("District: " + ev.describeDistrict(di));
          }
        }
    }
}

// Colored outlines of every district, plus the rectangle being dragged.
void GameView::drawDistricts(Painter& painter) {
  auto& ev = getWorld().evolution;
  auto outline = [&](int x0, int y0, int x1, int y1, Color color) {
    painter.setFillColor(color);
    auto tileAt = [&](int x, int y) {
      if(!inCity(MapPoint(x, y))) return;
      Vector2 pt = getScreenPoint(MapPoint(x, y));
      if(pt.x < -tileWidth || pt.x > getWidth() + tileWidth || pt.y < 0 || pt.y > getHeight() + 2 * tileHeight) return;
      Rect2D r(0, 0, tileWidth, tileHeight);
      pt.x -= tileWidth / 2; pt.y -= tileHeight; r.move(pt);
      fillDiamond(painter, r);
    };
    for(int x = x0; x <= x1; ++x) { tileAt(x, y0); tileAt(x, y1); }
    for(int y = y0 + 1; y < y1; ++y) { tileAt(x0, y); tileAt(x1, y); }
  };
  static const Color colors[6] = {Color(255, 255, 255, 120), Color(255, 140, 0, 140), Color(0, 200, 255, 140),
                                  Color(70, 210, 70, 140), Color(255, 220, 0, 140), Color(175, 95, 255, 140)};
  for(const auto& d : ev.districts) outline(d.x0, d.y0, d.x1, d.y1, colors[std::clamp(d.type, 0, 5)]);
  if(ev.mayorScope != 0) {
    int mx0, my0, mx1, my1; ev.mayorBounds(getWorld(), mx0, my0, mx1, my1);
    outline(mx0, my0, mx1 - 1, my1 - 1, Color(255, 70, 200, 150)); // the mayor's work area
  }
  if(districtMode && districtDrag)
    outline(std::min(districtStart.x, districtEnd.x), std::min(districtStart.y, districtEnd.y),
            std::max(districtStart.x, districtEnd.x), std::max(districtStart.y, districtEnd.y), Color(255, 255, 255, 200));
}

/*
 * Show informatiosn about selected Tool
 */

// Dynamic city life is rendered after terrain and before the cursor/HUD.
// Commodity transport remains the engine's real inventory simulation.
void GameView::updateStreetActors() {
  auto& world = getWorld();
  auto& e = world.evolution;
  int delta = lastAtmosphereTime < 0 ? 0 : (e.visualTime-lastAtmosphereTime+3600000)%3600000;
  lastAtmosphereTime = e.visualTime;
  delta = std::min(delta,250);
  auto road = [&world](MapPoint p) {
    if(!world.map.is_visible(p)) return false;
    int g = world.map(p)->getGroup();
    return g == GROUP_ROAD || g == GROUP_TRACK || g == GROUP_ROAD_BRIDGE || g == GROUP_TRACK_BRIDGE;
  };
  if(!e.pedestrians && !getConfig()->carsEnabled.get()) streetActors.clear();
  trafficRefresh -= delta;
  if(trafficRefresh <= 0 && (e.pedestrians || getConfig()->carsEnabled.get())) {
    trafficRefresh = 5000;
    std::vector<MapPoint> streets;
    for(auto* c : world.map.constructions) if(road(c->point)) {
      auto screen = getScreenPoint(c->point);
      if(screen.x > -100 && screen.y > -100 && screen.x < getWidth()+100 && screen.y < getHeight()+100)
        streets.push_back(c->point);
    }
    if(!streets.empty()) {
      size_t target = std::min<size_t>(72, streets.size()/2+1);
      for(size_t attempt=0;attempt<target*4&&streetActors.size()<target;++attempt) {
        unsigned int seed=unsigned(attempt*2246822519U+streetActors.size()*2654435761U+e.visualTime);
        MapPoint p=streets[seed%streets.size()];
        bool occupied=false;for(auto& actor:streetActors)occupied|=actor.point==p;
        if(occupied)continue;
        auto route=planStreetTrip(world,p,seed);if(route.size()<2)continue;
        StreetActor actor{p,p,p,0,streetActors.size()%3==0,seed,seed};actor.route=std::move(route);streetActors.push_back(std::move(actor));
      }
    }
  }
  for(auto it = streetActors.begin(); it != streetActors.end();) {
    auto& a = *it;
    if(!road(a.point)){it=streetActors.erase(it);continue;}
    if(!road(a.next)){a.next=a.point;a.progress=0;a.route=planStreetTrip(world,a.point,a.style++);a.routeIndex=0;}
    if(a.point==a.next || a.progress>=1.f) {
      if(a.point!=a.next){a.previous=a.point;a.point=a.next;a.progress-=1.f;}
      if(a.routeIndex>=a.route.size()){it=streetActors.erase(it);continue;}
      if(!road(a.route[a.routeIndex])){a.route=planStreetTrip(world,a.point,++a.style);a.routeIndex=0;}
      if(a.route.empty()){it=streetActors.erase(it);continue;}
      a.next=a.route[a.routeIndex++];
    }
    bool motor=TrafficEra::motorVehicle(world.tech_level,MAX_TECH_LEVEL,a.appearance);
    a.progress += delta/(a.car?(motor?1900.f:3600.f):5200.f);
    ++it;
  }
}

void GameView::drawStreetActors(Painter& painter, MapPoint tile) {
  const auto& e = getWorld().evolution;
  for(const auto& a : streetActors) {
    if((a.progress < .5f ? a.point : a.next) != tile) continue;
    if((a.car && !getConfig()->carsEnabled.get()) || (!a.car && !e.pedestrians)) { continue; }
    auto from=getScreenPoint(a.point), to=getScreenPoint(a.next);
    auto p=from+(to-from)*std::min(1.f,a.progress);
    p.y-=tileHeight*.52f;
    p.x+=a.car ? 0 : tileWidth*.19f;
    if(p.x<0 || p.y<0 || p.x>getWidth() || p.y>getHeight()) { continue; }
    float z=std::clamp(zoom,.4f,2.5f);
    if(a.car) {
      int dx=a.next.x-a.point.x,dy=a.next.y-a.point.y;
      const char* direction=dx>0?"SE":dy>0?"SW":dx<0?"NW":"NE";
      bool motor=TrafficEra::motorVehicle(getWorld().tech_level,MAX_TECH_LEVEL,a.appearance);
      const char* kind=motor?"car-":(a.appearance%2?"carriage-":"horse-walk-");
      auto texture=texture_manager->load(std::string("images/transit/")+kind+direction+".png");
      float width=texture->getWidth()*zoom,height=texture->getHeight()*zoom;
      Rect2D actorRect(p.x-width/2,p.y-height+4*zoom,p.x+width/2,p.y+4*zoom);
      texture->setLighting(255-e.darkness()*110/1000,255-e.darkness()*95/1000,255-e.darkness()*65/1000);
      painter.drawStretchTexture(texture,actorRect);texture->setLighting(255,255,255);
      drawNightLayers(painter,std::string("Traffic:")+kind+direction+":0",actorRect,true,true);
    } else {
      const bool woman=(a.appearance%2)==0;
      const int dx=a.next.x-a.point.x,dy=a.next.y-a.point.y;
      const char* direction=woman?(dx>0?"SE":dy>0?"SW":dx<0?"NW":"NE"):(dx>0||dy<0?"NE":"SW");
      auto texture=texture_manager->load(std::string("images/transit/")+(woman?"woman-":"pedestrian-")+direction+"-0"+std::to_string(int(a.progress*8)%4)+".png");
      texture->setLighting(255-e.darkness()*110/1000,255-e.darkness()*95/1000,255-e.darkness()*65/1000);
      painter.drawStretchTexture(texture,Rect2D(p.x-8*zoom,p.y-21*zoom,p.x+8*zoom,p.y+3*zoom));
      texture->setLighting(255,255,255);
    }
  }
}


void GameView::drawTransit(Painter& painter,int modeFilter,MapPoint tile) {
  auto& w=getWorld();auto& transit=w.transit;
  auto center=[this](MapPoint p){return getScreenPoint(p)-Vector2(0,tileHeight/2);};
  const int length=w.map.len();
  if(modeFilter==1) {
    for(int layer=0;layer<2;++layer) {
      if(layer==0 && transit.view!=2 && transit.view!=1)continue;
      const auto& cells=layer?transit.viaducts:transit.tunnels;
      for(int id:cells){MapPoint p(id%length,id/length);auto a=center(p);if(a.x < -tileWidth || a.x>getWidth()+tileWidth || a.y < -100 || a.y>getHeight()+100)continue;
        if(layer){auto top=a-Vector2(0,42*zoom);painter.setLineColor(Color(171,191,183));painter.drawLine(a,top);a=top;}
        painter.setLineColor(layer?Color(207,198,163):Color(53,210,207,180));
        for(MapPoint n:{p.e(),p.s()})if(w.map.is_visible(n)&&cells.count(n.y*length+n.x)) {
          auto b=center(n);if(layer)b-=Vector2(0,42*zoom);
          Vector2 tangent=b-a;float norm=std::sqrt(tangent.x*tangent.x+tangent.y*tangent.y);
          if(norm<=0)continue;Vector2 side(-tangent.y/norm*8*zoom,tangent.x/norm*8*zoom);
          if(layer) {
            Vector2 depth(0,6*zoom);Vector2 wall[]={a+side,b+side,b+side+depth,a+side+depth};
            painter.setFillColor(Color(84,103,103));painter.fillPolygon(4,wall);
          }
          Vector2 deck[]={a+side,b+side,b-side,a-side};
          painter.setFillColor(layer?Color(147,157,150):Color(22,96,104,210));painter.fillPolygon(4,deck);
          painter.setLineColor(layer?Color(83,92,84):Color(56,148,148));
          for(int k=0;k<5;++k){auto c=a+(b-a)*(k/5.f);painter.drawLine(c+side*.8f,c-side*.8f);}
          painter.setLineColor(layer?Color(231,220,180):Color(98,237,223));
          painter.drawLine(a+side*.55f,b+side*.55f);painter.drawLine(a-side*.55f,b-side*.55f);
          if(layer) {
            Vector2 ground=center(p);Vector2 pier[]={a+Vector2(-3*zoom,5*zoom),a+Vector2(3*zoom,5*zoom),ground+Vector2(3*zoom,0),ground+Vector2(-3*zoom,0)};
            painter.setFillColor(Color(125,144,139));painter.fillPolygon(4,pier);
          }
        }
        painter.setFillColor(layer?Color(151,169,161):Color(53,210,207,160));painter.fillRectangle(Rect2D(a.x-2*zoom,a.y-2*zoom,a.x+2*zoom,a.y+2*zoom));
      }
    }
  }
  const char* directions[]={"SE","SW","NW","NE"};
  for(size_t ri=0;ri<transit.routes.size();++ri){const auto& route=transit.routes[ri];bool raised=route.mode>=2;
    if((modeFilter==0 && raised)||(modeFilter==1 && !raised)||(route.mode==2 && transit.view!=2&&transit.view!=1))continue;
    int segments=int(route.path.size())-1;if(segments<1)continue;
    float progress=std::fmod(w.evolution.visualTime/(route.mode==0?650.f:420.f)+ri*4.f,2.f*segments);bool back=progress>segments;if(back)progress=2.f*segments-progress;
    int i=std::min(segments-1,int(progress));float f=progress-i;auto from=route.path[i],to=route.path[i+1];
    if(modeFilter==0 && tile!=(f<.5f?from:to))continue;
    auto pos=center(from)*(1-f)+center(to)*f;if(route.mode==3)pos-=Vector2(0,42*zoom);
    int dx=(to.x-from.x)*(back?-1:1),dy=(to.y-from.y)*(back?-1:1);int dir=dx>0?0:dy>0?1:dx<0?2:3;
    bool motor=TrafficEra::motorVehicle(w.tech_level,MAX_TECH_LEVEL,unsigned(ri*37));
    auto texture=texture_manager->load(std::string("images/transit/")+(route.mode==0?(motor?"bus-":"carriage-"):"train-")+directions[dir]+".png");
    float width=texture->getWidth()*zoom,height=texture->getHeight()*zoom;
    Rect2D actorRect(pos.x-width/2,pos.y-height+4*zoom,pos.x+width/2,pos.y+4*zoom);
    int darkness=w.evolution.darkness();texture->setLighting(255-darkness*110/1000,255-darkness*95/1000,255-darkness*65/1000);painter.drawStretchTexture(texture,actorRect);texture->setLighting(255,255,255);
    drawNightLayers(painter,std::string("Traffic:")+(route.mode==0?(motor?"bus-":"carriage-"):"train-")+directions[dir]+":0",actorRect,true,true);
  }
}

void GameView::drawCityLife(Painter& painter) {
  auto& world = getWorld();
  auto& e = world.evolution;
  const int weather=e.currentWeather();
  if(weather>=2) {
    const int drops=weather==3 ? 200 : 100;
    painter.setLineColor(Color(159,193,222,weather==3 ? 145 : 105));
    for(int i=0;i<drops;++i) {
      float x=std::fmod(i*137.31f + e.visualTime*.09f,getWidth()+30)-15;
      float y=std::fmod(i*97.73f + e.visualTime*.43f,getHeight()+50)-25;
      painter.drawLine(Vector2(x,y),Vector2(x-4,y+13));
    }
    if(weather==3 && e.visualTime%19000 < 90) {
      painter.setFillColor(Color(201,218,255,55));
      painter.fillRectangle(Rect2D(0,0,getWidth(),getHeight()));
    }
  }
}

void GameView::showToolInfo( int number /*= 0*/ )
{
    std::stringstream infotextstream;

    if( getUserOperation()->action == UserOperation::ACTION_QUERY ) //query
    {
        infotextstream << _("Query Tool: Show information about selected building.");
    }
    else if( getUserOperation()->action == UserOperation::ACTION_BULLDOZE ) //bulldoze
    {
        infotextstream << _("Bulldozer: remove building -price varies-");
    }
    else if( getUserOperation()->action == UserOperation::ACTION_BUILD )
    {
        infotextstream << getUserOperation()->constructionGroup->getName();
        infotextstream << _(": Cost to build ") << getUserOperation()->constructionGroup->getCosts(getWorld()) <<_("$");
        infotextstream << _(", to bulldoze ") << getUserOperation()->constructionGroup->bul_cost <<_("$") << ".";
        if( number > 1 ){
            infotextstream << _(" To build ") << number << _(" of them ");
            infotextstream << _("will cost about ") << number*getUserOperation()->constructionGroup->getCosts(getWorld()) << _("$") << "-";
        }
    }
    else if ( getUserOperation()->action == UserOperation::ACTION_EVACUATE )
    {
        infotextstream << _("Evacuation of commodities is for free.");
    }
    else if ( getUserOperation()->action == UserOperation::ACTION_FLOOD )
    {
        infotextstream <<  _("Water") << _(": Cost to build ") << GROUP_WATER_COST << _("$");
        infotextstream << _(", to bulldoze ") << GROUP_WATER_BUL_COST << _("$") << ".";
    }

    printStatusMessage( infotextstream.str() );
}

/*
 * Print a Message to the StatusBar.
 */
// TODO: this method should be moved to the Game class
void GameView::printStatusMessage( std::string message ){
    if( message == lastStatusMessage ){
        return;
    }
    Component* root = this;
    while( root->getParent() )
        root = root->getParent();
    Desktop* desktop = dynamic_cast<Desktop*> (root);
    if(!desktop) {
        std::cerr << "Root not a desktop!?!\n";
        return;
    }

    try {
        Paragraph* statusParagraph = getParagraph( *root, "statusParagraph");
        statusParagraph->setText( message );
    } catch(std::exception& e) {
        std::cerr << "Couldn't print status message '" << message  << "': "
            << e.what() << "\n";
        return;
    }
    lastStatusMessage = message;
}

int GameView::bulldozeCost( MapPoint tile ){

    if (!getWorld().map.is_visible(tile))
    {   return 0;}
    Construction *reportingConstruction = getWorld().map(tile)->reportingConstruction;
    if (reportingConstruction)
    {   return reportingConstruction->constructionGroup->bul_cost;}
    else
    {
        int group = getWorld().map(tile)->getGroup();
        if (group == GROUP_DESERT)
        {   return 0;}
        else if (group == GROUP_WATER)
        {   return GROUP_WATER_BUL_COST;}
        else
        {   return 1;}
    }
    return 0;
}

int GameView::buildCost(MapPoint tile) {
  Message::ptr tmp;
  if(!getUserOperation()->isAllowedHere(getWorld(), tile, tmp)
    || getUserOperation()->action == UserOperation::ACTION_QUERY
    || getUserOperation()->action == UserOperation::ACTION_EVACUATE
  )
    return 0;
  if(getUserOperation()->action == UserOperation::ACTION_BUILD) {
    if (getWorld().map(tile)->is_water()) //building a bridge
      return BRIDGE_FACTOR * getUserOperation()->constructionGroup->getCosts(getWorld());
    else //building on land
      return getUserOperation()->constructionGroup->getCosts(getWorld());
  }
  if(getUserOperation()->action == UserOperation::ACTION_FLOOD)
    return GROUP_WATER_COST;
  return 0;
}

//Register as Component
IMPLEMENT_COMPONENT_FACTORY(GameView)

/** @file lincity-ng/GameView.cpp */

void GameView::drawNightLayers(Painter& painter,const std::string& key,const Rect2D& rect,bool occupied,bool powered) {
  if(!occupied) return;
  auto it=nightCrops.find(key); if(it==nightCrops.end()) return;
  const auto& e=getWorld().evolution; float amount=e.darkness()/1000.f;
  if(amount<=0) return;
  float late=0;
  if(e.skyPhase==2) late=std::clamp(float(e.skyElapsed)/std::max(1,e.skyDuration)*2.f-.6f,0.f,1.f);
  for(const auto& layer:it->second) {
    if(layer.electric&&!powered)continue;
    float alpha=amount*(layer.evening?1-late:1); if(alpha<=0)continue;
    auto tx=texture_manager->load(layer.file);tx->setOpacity(static_cast<unsigned char>(255*alpha));
    Vector2 p=rect.p1+Vector2(layer.x*zoom,layer.y*zoom);
    painter.drawStretchTexture(tx,Rect2D(p.x,p.y,p.x+tx->getWidth()*zoom,p.y+tx->getHeight()*zoom));tx->setOpacity(255);
  }
}

void GameView::drawEmergencyActors(Painter& painter,MapPoint tile) {
 auto& w=getWorld();int n=w.map.len();auto point=[n](int id){return MapPoint(id%n,id/n);};
 for(const auto& u:w.civic.units){if(u.path.empty()||u.step>=int(u.path.size()))continue;int id=u.path[u.step];if(point(id)!=tile)continue;int next=u.step+1<int(u.path.size())?u.path[u.step+1]:id;auto a=point(id),b=point(next);int cost=w.civic.terrainCost(w,next,u.home,u.returning?u.home:next);float f=next==id?0:std::clamp(u.credit/float(std::max(1,cost)),0.f,1.f);auto p=getScreenPoint(a)*(1-f)+getScreenPoint(b)*f-Vector2(0,tileHeight*.52f);
  int dx=b.x-a.x,dy=b.y-a.y;const char* direction=dx>0?"SE":dy>0?"SW":dx<0?"NW":"NE";bool motor=TrafficEra::motorVehicle(w.tech_level,MAX_TECH_LEVEL,unsigned(u.home));std::string kind=motor?(u.kind==CivicServices::FIRE_CALL?"firetruck-":u.kind==CivicServices::MEDICAL_CALL?"ambulance-":"policecar-"):u.kind==CivicServices::FIRE_CALL?"pumpwagon-":"carriage-";
  std::string file=std::string("images/")+(!motor&&u.kind!=CivicServices::FIRE_CALL?"transit/":"emergency/")+kind+direction+".png";auto tx=texture_manager->load(file);float width=tx->getWidth()*zoom,height=tx->getHeight()*zoom;int dark=w.evolution.darkness();tx->setLighting(255-dark*110/1000,255-dark*95/1000,255-dark*65/1000);painter.drawStretchTexture(tx,Rect2D(p.x-width/2,p.y-height,p.x+width/2,p.y));tx->setLighting(255,255,255);
  if(motor&&w.evolution.visualTime%800<400){painter.setFillColor(Color(120,195,255));painter.fillRectangle(Rect2D(p.x-2*zoom,p.y-height*.8f,p.x+2*zoom,p.y-height*.8f+2*zoom));}
  if(dark>0){painter.setFillColor(Color(255,215,145));painter.fillRectangle(Rect2D(p.x+width*.25f,p.y-5*zoom,p.x+width*.25f+2*zoom,p.y-3*zoom));}
  if(u.kind==CivicServices::FIRE_CALL&&!u.returning&&next==id){painter.setLineColor(Color(161,211,237));painter.drawLine(p,p+Vector2(12*zoom,-16*zoom));}
 }
 auto* building=w.map(tile)->reportingConstruction;
 if(!building)return;
 int root=building->point.y*n+building->point.x;
 for(auto& c:w.civic.incidents){if(c.target!=root)continue;auto p=getScreenPoint(tile.e(building->constructionGroup->size-1))-Vector2(0,tileHeight);
  if(c.kind==CivicServices::FIRE_CALL&&building->constructionGroup->group!=GROUP_FIRE){auto* rg=ResourceGroup::resMap["Fire"];if(rg&&!rg->graphicsInfoVector.empty())drawTexture(painter,tile.e(building->constructionGroup->size-1),&rg->graphicsInfoVector[(w.evolution.visualTime/100)%rg->graphicsInfoVector.size()]);}
  if(c.kind==CivicServices::ZOMBIE_CALL){painter.setFillColor(Color(133,180,102));for(int i=0;i<std::min(4,c.severity);++i)painter.fillRectangle(Rect2D(p.x+i*4*zoom,p.y-10*zoom,p.x+(i*4+3)*zoom,p.y));}
 }
}
