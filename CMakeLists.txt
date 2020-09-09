cmake_minimum_required(VERSION 3.10)

project("Nara" C)

if ((CMAKE_C_COMPILER_ID MATCHES "Clang") OR (CMAKE_C_COMPILER_ID MATCHES "GNU"))
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g -std=c11 -Wall -Wextra -Wconversion -pedantic")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -mtune=generic")

elseif (CMAKE_C_COMPILER_ID MATCHES "MSVC")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Od /Zi /D_CRT_SECURE_NO_WARNINGS")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2 /D_CRT_SECURE_NO_WARNINGS")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif()

set(SOURCES
	"./source/engine/nara.c"
)

add_executable("nara" ${SOURCES})

# LibJapan
option(JA_BUILD_SHARED OFF)
option(JA_BUILD_TEST OFF)

add_subdirectory("./source/engine/libjapan")
target_link_libraries("nara" "japan-static")

target_include_directories("nara" PRIVATE "./source/engine/libjapan/include")

# LibKansai
option(KA_BUILD_SHARED OFF)
option(KA_BUILD_SKETCHES OFF)

add_subdirectory("./source/engine/libkansai")
target_link_libraries("nara" "kansai-static")

target_include_directories("kansai-static" PRIVATE "./source/engine/libjapan/include")
target_include_directories("nara" PRIVATE "./source/engine/libkansai/include")
