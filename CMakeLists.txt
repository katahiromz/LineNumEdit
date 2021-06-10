# CMakeLists.txt --- CMake project settings
#    ex) cmake -G "Visual Studio 9 2008" .
#    ex) cmake -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles" .
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.0)

# project name and language
project(LineNumEdit CXX RC)

##############################################################################

# LineNumEdit.dll
add_library(LineNumEdit SHARED LineNumEdit.cpp)
target_compile_definitions(LineNumEdit PRIVATE UNICODE _UNICODE LINENUMEDIT_IMPL LINENUMEDIT_DLL)
set_target_properties(LineNumEdit PROPERTIES PREFIX "")
set_target_properties(LineNumEdit PROPERTIES OUTPUT_NAME "LineNumEdit")
target_link_libraries(LineNumEdit shlwapi)

# libLineNumEdit_static.a
add_library(LineNumEdit_static STATIC LineNumEdit.cpp)
target_compile_definitions(LineNumEdit_static PRIVATE UNICODE _UNICODE LINENUMEDIT_IMPL)
target_link_libraries(LineNumEdit_static shlwapi)

# main.exe
add_executable(main WIN32 main.cpp main_res.rc)
if(FALSE)
    target_link_libraries(main comctl32 shlwapi LineNumEdit_static)
    target_compile_definitions(main PRIVATE UNICODE _UNICODE LINENUMEDIT_IMPL)
else()
    target_link_libraries(main comctl32 shlwapi)
    target_compile_definitions(main PRIVATE UNICODE _UNICODE LINENUMEDIT_DLL)
endif()

##############################################################################
