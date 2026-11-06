# SPDX-License-Identifier: GPL-2.0-or-later
# Keep C++ objects/allocations inside one compiler and runtime, rather than
# passing std::string, streams and exceptions into unrelated prebuilt DLLs.
add_library(expanded_fmt INTERFACE)
add_library(fmt::fmt ALIAS expanded_fmt)
target_include_directories(expanded_fmt SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/fmt/include")
target_compile_definitions(expanded_fmt INTERFACE FMT_HEADER_ONLY=1)

file(GLOB_RECURSE XMLPP_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/libxmlplusplus/libxml++/*.cc")
add_library(expanded_xmlpp STATIC ${XMLPP_SOURCES})
add_library(LibXml++-5.0::LibXml++-5.0 ALIAS expanded_xmlpp)
target_include_directories(expanded_xmlpp SYSTEM PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/libxmlplusplus")
target_compile_definitions(expanded_xmlpp PUBLIC LIBXMLXX_STATIC=1)
target_link_libraries(expanded_xmlpp PUBLIC LibXml2::LibXml2)
