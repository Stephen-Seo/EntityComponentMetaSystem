cmake_minimum_required(VERSION 3.22)
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
    EC/EC.hpp
    EC/ThreadPool.hpp
)

set(WillFailCompile_SOURCES
    test/WillFailCompileTest.cpp)

find_package(Threads REQUIRED)

add_library(EntityComponentSystem INTERFACE)
target_link_libraries(EntityComponentSystem INTERFACE ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(EntityComponentSystem INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})


include_directories(${CMAKE_CURRENT_SOURCE_DIR})


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -D NDEBUG")

target_compile_features(EntityComponentSystem INTERFACE cxx_std_14)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Debug', none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()


install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/EC DESTINATION include)


find_package(GTest)
if(GTEST_FOUND)
    set(UnitTests_SOURCES
        test/MetaTest.cpp
        test/ECTest.cpp
        test/ThreadPoolTest.cpp
        test/Main.cpp
    )

    add_executable(UnitTests ${UnitTests_SOURCES})
    target_link_libraries(UnitTests EntityComponentSystem ${GTEST_LIBRARIES})
    target_include_directories(UnitTests PUBLIC ${GTEST_INCLUDE_DIR})
    target_compile_features(UnitTests PUBLIC cxx_std_14)

    enable_testing()
    add_test(NAME UnitTests COMMAND UnitTests)
endif()

add_executable(WillFailCompile ${WillFailCompile_SOURCES})
set_target_properties(WillFailCompile PROPERTIES
    EXCLUDE_FROM_ALL True
    EXCLUDE_FROM_DEFAULT_BUILD True)
add_test(NAME WillFailCompile_0
    COMMAND ${CMAKE_COMMAND} --build . --target WillFailCompile --config $<CONFIGURATION>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(WillFailCompile_0 PROPERTIES WILL_FAIL True)
