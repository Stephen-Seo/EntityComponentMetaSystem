cmake_minimum_required(VERSION 3.0)
project(EntityComponentSystem)

set(EntityComponentSystem_HEADERS
    EC/Meta.hpp)

add_library(EntityComponentSystem INTERFACE)
target_include_directories(EntityComponentSystem INTERFACE ${CMAKE_SOURCE_DIR})


include_directories(${CMAKE_SOURCE_DIR})


set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -D NDEBUG")

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
    if(NOT (GCC_VERSION VERSION_GREATER 5.0 OR GCC_VERSION VERSION_EQUAL 5.0))
        message(FATAL_ERROR "${PROJECT_NAME} requires g++ 5.0 or greater.")
    endif ()
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
elseif(MSVC)
else()
    message(FATAL_ERROR "Your C++ compiler does not support C++14.")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Debug', none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()


install(FILES ${EntityComponentSystem_HEADERS} DESTINATION include/EC)


find_package(GTest)
if(GTEST_FOUND)
    set(UnitTests_SOURCES
        test/MetaTest.cpp
        test/Main.cpp)

    add_executable(UnitTests ${UnitTests_SOURCES})
    target_link_libraries(UnitTests EntityComponentSystem ${GTEST_LIBRARIES})

    enable_testing()
    add_test(NAME UnitTests COMMAND UnitTests)
endif()

