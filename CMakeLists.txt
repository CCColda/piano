cmake_minimum_required(VERSION 3.0.0)
project(piano
	VERSION      1.0.0
	DESCRIPTION  "Piano"
	LANGUAGES    "CXX"
)

set(
	PIANO_SOURCES
	src/main.cpp
	src/app_graphics.cpp
	src/app_serial_thread.cpp
	src/app_audio_thread.cpp
	src/app.cpp
	src/serial_parser.cpp
	src/sounds.cpp
	src/serial.cpp
	src/windows_serial.cpp
	src/audio.cpp
	src/notes.cpp
	src/serial_notes.cpp
)

set(NEON_BUILD_PLATFORM ON CACHE BOOL "")
set(NEON_COPY_DATA_FILES ON CACHE BOOL "")
set(CMAKE_BUILD_TYPE DEBUG)
option(PIANO_BUILD_WITH_FLUIDSYNTH OFF "Build with fluidsynth for midi playback")
option(PIANO_BUILD_WITH_OPENAL ON "Build with OpenAL")

set(PIANO_MIDI_ENABLED 0)
set(PIANO_AL_ENABLED 0)

if (PIANO_BUILD_WITH_FLUIDSYNTH)
	find_package(FluidSynth)
	if (NOT FluidSynth_FOUND)
		message(WARNING "FluidSynth was not found. Midi playback won't be enabled.")
	else()
		set(PIANO_MIDI_ENABLED 1)
	endif()
endif()

if (PIANO_BUILD_WITH_OPENAL)
	find_package(OpenAL)

	if (NOT OpenAL_FOUND)
		message(WARNING "OpenAL was not found. Simple playback won't be enabled.")
	else()
		set(PIANO_AL_ENABLED 1)
	endif()
endif()

if (NOT PIANO_AL_ENABLED AND NOT PIANO_MIDI_ENABLED)
	message(FATAL_ERROR "Can't build without sound support.")
endif()

include(FetchContent)

FetchContent_Declare(
	CLI11
	GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
	GIT_TAG        v2.3.2
)

FetchContent_Declare(
	Midifile
	GIT_REPOSITORY https://github.com/craigsapp/midifile
	GIT_TAG        master
)

FetchContent_MakeAvailable(CLI11)

FetchContent_GetProperties(Midifile)
if(NOT Midifile_POPULATED)
	message(VERBOSE "Populating Midifile library")
	FetchContent_Populate(Midifile)
endif()

add_custom_target(
	MidifileBuild ALL
	COMMAND ${CMAKE_MAKE_PROGRAM} library
	BYPRODUCTS ${midifile_SOURCE_DIR}/lib/libmidifile.a
	WORKING_DIRECTORY ${midifile_SOURCE_DIR}
	COMMENT "Building Midifile library..."
)

add_library(Midifile::Midifile STATIC IMPORTED)
add_dependencies(Midifile::Midifile MidifileBuild)
set_property(TARGET Midifile::Midifile PROPERTY IMPORTED_LOCATION ${midifile_SOURCE_DIR}/lib/libmidifile.a)

add_subdirectory(lib/neon)

add_executable(piano)
add_dependencies(piano Midifile::Midifile)

target_include_directories(piano PRIVATE ${NEON_INCLUDE_DIR} ${midifile_SOURCE_DIR}/include)
target_sources(piano PRIVATE ${PIANO_SOURCES})
target_compile_definitions(piano PRIVATE PIANO_MIDI_ENABLED=${PIANO_MIDI_ENABLED} PIANO_AL_ENABLED=${PIANO_AL_ENABLED})
target_compile_features(piano PRIVATE cxx_std_17)
target_link_libraries(piano PRIVATE CLI11::CLI11 colda::neon colda::neon::platform Midifile::Midifile)

if (${PIANO_MIDI_ENABLED})
	target_link_libraries(piano PRIVATE FluidSynth::libfluidsynth)
endif()

if (${PIANO_AL_ENABLED})
	target_include_directories(piano PRIVATE ${OPENAL_INCLUDE_DIR})
	target_link_libraries(piano PRIVATE OpenAL::OpenAL)
endif()

add_custom_command(
	TARGET piano
	POST_BUILD
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy_directory data ${NEON_COPY_DATA_FILES_DESTINATION}
	COMMENT "Copying data files..."
)
