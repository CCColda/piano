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

find_package(OpenAL REQUIRED)

include(FetchContent)

FetchContent_Declare(
	CLI11
	GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
	GIT_TAG        v2.3.2
)

FetchContent_MakeAvailable(CLI11)

set(NEON_BUILD_PLATFORM ON)
set(NEON_COPY_DATA_FILES ON)
set(CMAKE_BUILD_TYPE DEBUG)

add_subdirectory(lib/neon)

add_executable(piano)
target_include_directories(piano PRIVATE ${OPENAL_INCLUDE_DIR} ${NEON_INCLUDE_DIR})
target_sources(piano PRIVATE ${PIANO_SOURCES})
target_compile_features(piano PRIVATE cxx_std_17)
target_link_libraries(piano PRIVATE CLI11::CLI11 OpenAL::OpenAL colda::neon colda::neon::platform)

