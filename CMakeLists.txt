
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
find_program(BISON bison)

message("### It´s dangerous to compile alone, read this: '${RUBY}'")
message("### It´s dangerous to compile alone, read this: '${RAKE}'")
message("### It´s dangerous to compile alone, read this: '${BISON}'")

if (NOT RUBY OR NOT RAKE OR NOT BISON)
	message(FATAL_ERROR "Install da deps' my final message. Goodbye.")
endif()

#execute_process(COMMAND rake -f "${PROJECT_SOURCE_DIR}/source/engine/mruby/Rakefile" ERROR_VARIABLE mruby_out)
#target_link_libraries("nara" PRIVATE "${PROJECT_SOURCE_DIR}/source/engine/mruby/build/host/lib/libmruby.a")

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
