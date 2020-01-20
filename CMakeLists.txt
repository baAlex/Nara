cmake_minimum_required(VERSION 3.10)

project("Nara" C)
include_directories("./source/lib-japan/include/")
include_directories("./source/lib-glad/")
include_directories("./source/lib-samplerate/src")

if ((CMAKE_C_COMPILER_ID MATCHES "Clang") OR (CMAKE_C_COMPILER_ID MATCHES "GNU"))
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -DJA_EXPORT_SYMBOLS")

	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DCPU_CLIPS_POSITIVE=0 -DCPU_CLIPS_NEGATIVE=0")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} '-DPACKAGE=\"NaraLibSamplerate\"' '-DVERSION=\"NaraCustom\"'")

	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g -std=c11 -Wall -DJA_DEBUG")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -flto -mtune=generic")

elseif (CMAKE_C_COMPILER_ID MATCHES "MSVC")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /DJA_EXPORT_SYMBOLS")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Od /Zi /DJA_DEBUG /D_CRT_SECURE_NO_WARNINGS")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2 /D_CRT_SECURE_NO_WARNINGS")

endif()

set(JAPAN_SOURCES
	"./source/lib-japan/source/aabounding.c"
	"./source/lib-japan/source/buffer.c"
	"./source/lib-japan/source/dictionary.c"
	"./source/lib-japan/source/endianness.c"
	"./source/lib-japan/source/list.c"
	"./source/lib-japan/source/matrix.c"
	"./source/lib-japan/source/status.c"
	"./source/lib-japan/source/tree.c"
	"./source/lib-japan/source/utilities.c"
	"./source/lib-japan/source/vector.c"
	"./source/lib-japan/source/configuration/arguments.c"
	"./source/lib-japan/source/configuration/configuration.c"
	"./source/lib-japan/source/configuration/files.c"
	"./source/lib-japan/source/image/format-sgi.c"
	"./source/lib-japan/source/image/image.c"
	"./source/lib-japan/source/sound/format-au.c"
	"./source/lib-japan/source/sound/format-wav.c"
	"./source/lib-japan/source/sound/laws.c"
	"./source/lib-japan/source/sound/sound.c")

set(GLAD_SOURCES "./source/lib-glad/glad.c")

set(SAMPLERATE_SOURCES
	"./source/lib-samplerate/src/samplerate.c"
	"./source/lib-samplerate/src/src_linear.c"
	"./source/lib-samplerate/src/src_sinc.c"
	"./source/lib-samplerate/src/src_zoh.c")

set(NARA_SOURCES
	"./source/engine/context/context.c"
	"./source/engine/context/input.c"
	"./source/engine/context/objects.c"
	"./source/engine/context/state.c"
	"./source/engine/mixer/mixer.c"
	"./source/engine/mixer/sample.c"
	"./source/engine/entity.c"
	"./source/engine/misc.c"
	"./source/engine/nterrain.c"
	"./source/engine/timer.c"
	"./source/game/camera.c"
	"./source/game/nara.c"
	"./source/game/point.c")

add_custom_command(
	OUTPUT  ./terrain-fragment.c
	COMMAND python3 ${CMAKE_SOURCE_DIR}/source/c-ify.py ${CMAKE_SOURCE_DIR}/source/engine/terrain-fragment.glsl ./terrain-fragment.c
)

add_custom_command(
	OUTPUT  ./terrain-vertex.c
	COMMAND python3 ${CMAKE_SOURCE_DIR}/source/c-ify.py ${CMAKE_SOURCE_DIR}/source/engine/terrain-vertex.glsl ./terrain-vertex.c
)

set(SDK_SOURCES
	"./source/sdk/erosion.c"
	"./source/sdk/sdk.c"
	"./source/sdk/simplex.c")

add_executable("nara"
	${JAPAN_SOURCES}
	${GLAD_SOURCES}
	${SAMPLERATE_SOURCES}
	${NARA_SOURCES}
	./terrain-fragment.c
	./terrain-vertex.c)

target_link_libraries("nara" "m")
target_link_libraries("nara" "glfw")
target_link_libraries("nara" "portaudio")

add_executable("sdk" ${JAPAN_SOURCES} ${SDK_SOURCES})
target_link_libraries("sdk" "m")
