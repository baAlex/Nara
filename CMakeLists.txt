
cmake_minimum_required(VERSION 3.10)
project("Nara" C)

if (MSVC)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif ()

add_executable("nara"
	"./source/engine/nara.c"
	"./source/engine/skydome.c"
)

# mruby
option(MRUBY_SHARED OFF)

add_subdirectory("./source/engine/mruby")
target_link_libraries("nara" PRIVATE "mruby-static")
target_include_directories("nara" PRIVATE "./source/engine/mruby/include")

# LibJapan
option(JAPAN_SHARED OFF)
option(JAPAN_BUILD_TEST OFF)

add_subdirectory("./source/engine/libjapan")
target_link_libraries("nara" PRIVATE "japan-static")
target_include_directories("nara" PRIVATE "./source/engine/libjapan/include")

# LibKansai
option(KANSAI_SHARED OFF)
option(KANSAI_BUILD_SKETCHES OFF)

add_subdirectory("./source/engine/libkansai")
target_link_libraries("nara" PRIVATE "kansai-static")
target_include_directories("nara" PRIVATE "./source/engine/libkansai/include")
