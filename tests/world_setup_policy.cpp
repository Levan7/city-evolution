#include "lincity/world_setup.hpp"
#include <cassert>
#include <iostream>
int main() {
  using namespace WorldSetup;
  for(int side = -100; side <= 11000; ++side) {
    int larger=nextSize(side,true), smaller=nextSize(side,false);
    assert(larger>=50 && larger<=400 && smaller>=50 && smaller<=400);
    if(side>=50 && side<=400) { assert(larger>=side); assert(smaller<=side); }
    assert(boundedSize(side)>=50 && boundedSize(side)<=400);
  }
  assert(nextSize(100,true)==150);
  assert(nextSize(100,false)==50);
  assert(nextSize(400,true)==400);
  assert(nextSize(50,false)==50);
  assert(nextSize(125,true)==150);
  assert(nextSize(125,false)==100);
  assert(label(100)=="Medium-small  100 x 100");
  assert(label(400)=="Vast  400 x 400");
  assert(sizes.back().side*sizes.back().side==16*100*100);
  assert(altitudeStep(0,0)==1);
  assert(altitudeStep(50,55)==1);
  assert(altitudeStep(0,100)==10);
  assert(emptyStartMoney==200000);
  std::cout << "PASS: world-size bounds/progression across 11,101 inputs, presets, budget and flat-map altitude safety\n";
}
