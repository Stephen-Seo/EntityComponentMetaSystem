
# Compiling

Create a build directory.  
`mkdir build`

Generate makefile with CMake.  
`cd build`  
`cmake -DCMAKE_INSTALL_PREFIX=./install -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=True ../src`

Build the project.  
`make`

Optionally install the project to where CMAKE\_INSTALL\_PREFIX was set.  
`make install`

