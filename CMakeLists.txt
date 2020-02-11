cmake_minimum_required(VERSION 3.10)

project(Nara C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

set(NARA_SOURCES
	"./source/engine/context/context.c"
	"./source/engine/context/input.c"
	"./source/engine/context/objects.c"
	"./source/engine/context/state.c"
	"./source/engine/mixer/dsp.c"
	"./source/engine/mixer/mixer.c"
	"./source/engine/mixer/sample.c"
	"./source/engine/vm/definitions.c"
	"./source/engine/vm/vm.c"
	"./source/engine/misc.c"
	"./source/engine/nara.c"
	"./source/engine/nterrain.c"
	"./source/engine/timer.c")

set(SDK_SOURCES
	"./source/sdk/erosion.c"
	"./source/sdk/sdk.c"
	"./source/sdk/simplex.c")

add_subdirectory("./source/lib-glad")
add_subdirectory("./source/lib-japan")
add_subdirectory("./source/lib-samplerate")

add_custom_command(
	OUTPUT  ./source/debug-fragment.c
	COMMAND python3 ${CMAKE_SOURCE_DIR}/source/c-ify.py ${CMAKE_SOURCE_DIR}/source/engine/debug-fragment.glsl ./source/debug-fragment.c)

add_custom_command(
	OUTPUT  ./source/debug-vertex.c
	COMMAND python3 ${CMAKE_SOURCE_DIR}/source/c-ify.py ${CMAKE_SOURCE_DIR}/source/engine/debug-vertex.glsl ./source/debug-vertex.c)

add_custom_command(
	OUTPUT  ./source/terrain-fragment.c
	COMMAND python3 ${CMAKE_SOURCE_DIR}/source/c-ify.py ${CMAKE_SOURCE_DIR}/source/engine/terrain-fragment.glsl ./source/terrain-fragment.c)

add_custom_command(
	OUTPUT  ./source/terrain-vertex.c
	COMMAND python3 ${CMAKE_SOURCE_DIR}/source/c-ify.py ${CMAKE_SOURCE_DIR}/source/engine/terrain-vertex.glsl ./source/terrain-vertex.c)

add_custom_target(
	game
	ALL
	COMMAND mrbc -o ./game.mrb
		${CMAKE_SOURCE_DIR}/source/game/common.rb
		${CMAKE_SOURCE_DIR}/source/game/camera.rb
		${CMAKE_SOURCE_DIR}/source/game/player.rb
		${CMAKE_SOURCE_DIR}/source/game/point.rb)

####

add_executable(nara
	${NARA_SOURCES}
	"./source/debug-fragment.c"
	"./source/debug-vertex.c"
	"./source/terrain-fragment.c"
	"./source/terrain-vertex.c")

target_link_libraries(nara "glad")
target_link_libraries(nara "japan")
target_link_libraries(nara "samplerate")

target_include_directories(nara PRIVATE "./source/lib-glad/")
target_include_directories(nara PRIVATE "./source/lib-japan/include")
target_include_directories(nara PRIVATE "./source/lib-samplerate/src")

target_link_libraries(nara "m")         # TODO
target_link_libraries(nara "glfw")      # "
target_link_libraries(nara "portaudio") # "
target_link_libraries(nara "mruby")     # "

add_executable(sdk ${SDK_SOURCES})

target_link_libraries(sdk "japan")
target_link_libraries(sdk "m")

target_include_directories(sdk PRIVATE "./source/lib-japan/include")
