cmake_minimum_required(VERSION 3.2)
set(CMAKE_SUPPRESS_REGENERATION true)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Editor)
cmake_policy(SET CMP0091 NEW)

project(Space-Survival-Game)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

add_compile_definitions(GLM_FORCE_SWIZZLE GLM_FORCE_RADIANS GLM_ENABLE_EXPERIMENTAL)

add_subdirectory(vendor)
add_subdirectory(Engine)
add_subdirectory(Game)
add_subdirectory(AllocatorTest)
add_subdirectory(Editor)

