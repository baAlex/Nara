
cmake_minimum_required(VERSION 3.10)
project("Nara" C)

if (MSVC)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif ()

add_executable("nara"
	"./source/engine/nara.c"
	"./source/engine/skydome.c"
)

# MRuby
find_program(RUBY ruby)
find_program(RAKE rake)

message("### It´s dangerous to compile alone, read this: '${RUBY}'")
message("### It´s dangerous to compile alone, read this: '${RAKE}'")

if (NOT RUBY OR NOT RAKE)
	message(FATAL_ERROR "Install da deps' my final message. Goodbye.")
endif()

if (MSVC) # Hack just to force rake to use MSVC
	add_custom_command(TARGET "nara" PRE_BUILD COMMAND ${RAKE} "VSINSTALLDIR=YesPlease"
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/source/engine/mruby")
else ()
	add_custom_command(TARGET "nara" PRE_BUILD COMMAND ${RAKE}
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/source/engine/mruby")
endif ()

target_link_libraries("nara" PRIVATE
	"${PROJECT_SOURCE_DIR}/source/engine/mruby/build/host/lib/libmruby${CMAKE_STATIC_LIBRARY_SUFFIX}")
target_include_directories("nara" PRIVATE "${PROJECT_SOURCE_DIR}/source/engine/mruby/include")

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
