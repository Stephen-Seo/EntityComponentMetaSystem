cmake_minimum_required(VERSION 3.0)
project(EntityComponentSystem)

set(EntityComponentSystem_HEADERS
    EC/Meta/Combine.hpp
    EC/Meta/Contains.hpp
    EC/Meta/ContainsAll.hpp
    EC/Meta/ForEach.hpp
    EC/Meta/IndexOf.hpp
    EC/Meta/Matching.hpp
    EC/Meta/Morph.hpp
    EC/Meta/TypeList.hpp
    EC/Meta/TypeListGet.hpp
    EC/Meta/Meta.hpp
    EC/Bitset.hpp
    EC/Manager.hpp
    EC/EC.hpp)

add_library(EntityComponentSystem INTERFACE)
target_include_directories(EntityComponentSystem INTERFACE ${CMAKE_SOURCE_DIR})


include_directories(${CMAKE_SOURCE_DIR})


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -D NDEBUG")

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT (GCC_VERSION VERSION_GREATER 5.0 OR GCC_VERSION VERSION_EQUAL 5.0))
        message(FATAL_ERROR "${PROJECT_NAME} requires g++ 5.0 or greater.")
    endif ()
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE CLANG_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT (CLANG_VERSION VERSION_GREATER 3.4 OR CLANG_VERSION VERSION_EQUAL 3.4))
        message(FATAL_ERROR "${PROJECT_NAME} requires clang 3.4 or greater.")
    endif()
elseif(MSVC)
else()
    message(FATAL_ERROR "Your C++ compiler does not support C++14.")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Debug', none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()


install(DIRECTORY ${CMAKE_SOURCE_DIR}/EC DESTINATION include)


find_package(GTest)
if(GTEST_FOUND)
    set(UnitTests_SOURCES
        test/MetaTest.cpp
        test/ECTest.cpp
        test/Main.cpp)

    add_executable(UnitTests ${UnitTests_SOURCES})
    target_link_libraries(UnitTests EntityComponentSystem ${GTEST_LIBRARIES})
    target_include_directories(UnitTests PUBLIC ${GTEST_INCLUDE_DIR})

    enable_testing()
    add_test(NAME UnitTests COMMAND UnitTests)
endif()

