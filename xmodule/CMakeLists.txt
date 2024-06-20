cmake_minimum_required(VERSION 3.10)
project(xlib)
set(ROOT_DIR ${PROJECT_SOURCE_DIR}/)
message(STATUS "ROOT_DIR: " ${ROOT_DIR})

## compiler
#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -std=c++11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast")
#SET(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
#SET(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
SET(CMAKE_C_COMPILER gcc)
SET(CMAKE_CXX_COMPILER g++)

## options
set(CMAKE_BUILD_TYPE debug)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g ")
#set(CMAKE_BUILD_TYPE release)
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 ")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 ")
add_compile_options(-DLINUX_PLATFORM -DADRV9025 -DMAKE_XLIB -DROCKCHIP_BOARD -Wno-unused-result)
#add_compile_options(-Wchar-subscripts -Wno-format-overflow)
message(STATUS "CMAKE_C_FLAGS: " ${CMAKE_C_FLAGS})
#get_target_property(MY_COMPILE_OPTIONS ${PROJECT_NAME} COMPILE_OPTIONS)
#message(STATUS "MY_COMPILE_OPTIONS  = ${MY_COMPILE_OPTIONS}")

## include
include_directories(${ROOT_DIR}/include)


## link
#link_directories(${ROOT_DIR}/lib)
#LINK_LIBRARIES(${ROOT_DIR}/aa.a)
#LINK_LIBRARIES(${ROOT_DIR}/lteSync/bb.a)

## sources
#aux_source_directory(${ROOT_DIR}/app/g73c sources)
#file(GLOB ${ROOT_DIR}/boards/daughterboards/adrv9025 CONFIGURE_DEPENDS *.c *.h)
#file(GLOB_RECURSE sources ${ROOT_DIR}/app/g73c CONFIGURE_DEPENDS *.c *.h)
file(GLOB_RECURSE 
	sources 
	${ROOT_DIR}/src/*.c
)
aux_source_directory(${ROOT_DIR}/algo sources)

## target
#add_executable(app_main)
add_library(xlib ${sources})
#target_sources(xlib PUBLIC ${sources})
#target_link_libraries(xlib m rt pthread)


