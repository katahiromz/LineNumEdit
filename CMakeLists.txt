# CMakeLists.txt --- CMake project settings
#    ex) cmake -G "Visual Studio 9 2008" .
#    ex) cmake -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles" .
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.0)

# project name and language
project(LineNumEdit CXX RC)

##############################################################################

# main.exe uses superclassing?
option(LINENUMEDIT_SUPERCLASSING "Use LineNumEdit superclassing" ON)
if(LINENUMEDIT_SUPERCLASSING)
    add_definitions(-DLINENUMEDIT_SUPERCLASSING)
endif()

# LineNumEdit.dll
add_library(LineNumEdit SHARED LineNumEdit.cpp)
target_compile_definitions(LineNumEdit PRIVATE UNICODE _UNICODE)
set_target_properties(LineNumEdit PROPERTIES PREFIX "")
set_target_properties(LineNumEdit PROPERTIES OUTPUT_NAME "LineNumEdit")

# main.exe
add_executable(main WIN32 main.cpp main_res.rc)
target_link_libraries(main comctl32)
target_compile_definitions(main PRIVATE UNICODE _UNICODE)

##############################################################################
