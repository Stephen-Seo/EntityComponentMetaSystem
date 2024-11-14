cmake_minimum_required(VERSION 3.7)
project(ECMS_Conan_Example)

set(ECMS_Conan_Example_SOURCES
    src/main.cpp
)

add_compile_options(
    -Wall -Wextra -Wpedantic -Wno-missing-braces
    $<$<COMPILE_LANGUAGE:CXX>:-Weffc++>
    $<$<CONFIG:DEBUG>:-Og>
)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Debug', none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

find_package(ecms CONFIG REQUIRED)

add_executable(ECMS_Conan_Example ${ECMS_Conan_Example_SOURCES})
target_link_libraries(ECMS_Conan_Example ecms::ecms)
