cmake_minimum_required(VERSION 3.15)
project(Sokobee VERSION 0.0.0)

set(CMAKE_C_STANDARD 17)

list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/sdl2")
list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/sdl2_mixer")

find_package(SDL2       CONFIG REQUIRED)
find_package(SDL2_ttf   CONFIG REQUIRED)
find_package(SDL2_mixer CONFIG REQUIRED)

file(GLOB_RECURSE SOURCE_FILES "Source/*.c" "Source/*.h")
add_executable(Sokobee ${SOURCE_FILES})

target_link_libraries(Sokobee PRIVATE
        SDL2::SDL2
        SDL2::SDL2main
        SDL2_ttf::SDL2_ttf
        SDL2_mixer::SDL2_mixer
)