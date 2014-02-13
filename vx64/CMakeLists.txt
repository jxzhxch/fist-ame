cmake_minimum_required(VERSION 2.8)
PROJECT(vx64)

IF(MSVC)
message(FATAL_ERROR "qemu can not compile in msvc yet!")
ENDIF()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

add_definitions(-DWANT_ICEBP)

IF(WIN32 AND (CMAKE_SIZEOF_VOID_P EQUAL 8))
add_definitions(-D_WIN64)
ENDIF()

include_directories(qemu)
include_directories(qemu/include)
include_directories(qemu/tcg)
include_directories(qemu/target-i386)

add_subdirectory(qemu)

set(vx64_SRCS vx64.c vx64.def)

ADD_LIBRARY(vx64 SHARED ${vx64_SRCS})
target_link_libraries(vx64 qemu)
IF(WIN32 AND NOT MSVC)
	set_target_properties(vx64 PROPERTIES LINK_FLAGS -static-libgcc)
	add_custom_command(TARGET vx64 POST_BUILD 
		COMMAND lib.exe /def:vx64.def /out:${PROJECT_BINARY_DIR}/libvx64.lib
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
ENDIF()


