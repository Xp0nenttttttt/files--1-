cmake_minimum_required(VERSION 3.15)
project(PlayerTitles LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Geode SDK root directory (set this when configuring CMake)
if(NOT DEFINED GEODE_DIR)
    message(FATAL_ERROR "Please define GEODE_DIR to your Geode SDK root directory, e.g. -DGEODE_DIR=path/to/Geode")
endif()

find_path(GEODE_INCLUDE_DIR
    NAMES Geode/Geode.hpp
    HINTS
        ${GEODE_DIR}/include
        ${GEODE_DIR}
)

if(NOT GEODE_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find Geode headers. Check GEODE_DIR and try again.")
endif()

set(MOD_ID "votrenom.player-titles")
set(MOD_SOURCES
    main.cpp
    AdminLayer.cpp
    RankingLayer.cpp
)

add_library(${MOD_ID} SHARED ${MOD_SOURCES})

target_include_directories(${MOD_ID} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${GEODE_INCLUDE_DIR}
)

target_compile_definitions(${MOD_ID} PRIVATE GEODE_MOD)

target_compile_features(${MOD_ID} PRIVATE cxx_std_20)

# Preserve the mod ID as the output filename, without an extra lib prefix.
set_target_properties(${MOD_ID} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "${MOD_ID}"
)

# Optional: allow overriding the output name if needed
if(DEFINED MOD_OUTPUT_NAME)
    set_target_properties(${MOD_ID} PROPERTIES OUTPUT_NAME "${MOD_OUTPUT_NAME}")
endif()
