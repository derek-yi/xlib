cmake_minimum_required(VERSION 3.10)
project(apps.bin)
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
add_compile_options(-DLINUX_PLATFORM -DMAKE_XLIB -Wno-unused-result)
#add_compile_options(-Wchar-subscripts -Wno-format-overflow)
message(STATUS "CMAKE_C_FLAGS: " ${CMAKE_C_FLAGS})
#get_target_property(MY_COMPILE_OPTIONS ${PROJECT_NAME} COMPILE_OPTIONS)
#message(STATUS "MY_COMPILE_OPTIONS  = ${MY_COMPILE_OPTIONS}")

## include
include_directories(${ROOT_DIR}/../include)

## link
#link_directories(${ROOT_DIR}/lib)
LINK_LIBRARIES(${ROOT_DIR}/../build/libxlib.a)

## sources
#aux_source_directory(${ROOT_DIR}/app/xx sources)
#file(GLOB ${ROOT_DIR}/boards/xx CONFIGURE_DEPENDS *.c *.h)
#file(GLOB_RECURSE sources ${ROOT_DIR}/app/xx CONFIGURE_DEPENDS *.c *.h)
file(GLOB_RECURSE 
	sources 
	${ROOT_DIR}/main/*.c
)
#aux_source_directory(${ROOT_DIR}/app/xx sources)

## target
add_executable(apps.bin)
target_sources(apps.bin PUBLIC ${sources})
target_link_libraries(apps.bin m rt pthread)


