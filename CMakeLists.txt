cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(TouchRemap)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

include_directories(
)

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)

if (NOT ${RELEASE})
  add_definitions(-DENABLE_LOGGING)
endif()

add_executable(TouchRemap
  main.c
  renderer.c
)

target_link_libraries(TouchRemap
  taihen_stub
  SceLibKernel_stub
  SceTouch_stub
  SceDisplay_stub
  SceCtrl_stub
  k
  gcc
)

set_target_properties(TouchRemap
  PROPERTIES LINK_FLAGS "-nostdlib"
)

vita_create_self(TouchRemap.suprx TouchRemap
  CONFIG ${CMAKE_SOURCE_DIR}/TouchRemap.yml
)
