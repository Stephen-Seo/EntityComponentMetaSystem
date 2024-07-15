# About

EntityComponentMetaSystem is a header-only library. However, UnitTests can be
built and run using cmake (gtest is ~~a dependency~~ no longer a dependency).

# Generated Doxygen Documentation

[Check this repository's gh-pages documentation on ECMS](https://stephen-seo.github.io/EntityComponentMetaSystem/)

[Alternatively, check out the doxygen docs hosted on my website](https://seodisparate.com/ecms_docs/)

# Compiling the UnitTests

Create a build directory.  
`mkdir build`

Generate makefile with CMake.  
`cd build`  
`cmake -DCMAKE_BUILD_TYPE=Debug ../src`

Build the project's UnitTests.  
`make`

Run the UnitTests.  
`./UnitTests`

# Install the Header-Only Library

`mkdir build; cd build`  
`cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release ../src`

Install the project to where you want to.  
`make DESTDIR=install_here install`

In this example, `CMAKE_INSTALL_PREFIX=/usr`, then invoking
`make DESTDIR=install_here install` will install the `src/EC` directory to
`install_here/usr/include`. The path to `Manager.hpp` will then look like
`install_here/usr/include/EC/Manager.hpp`
