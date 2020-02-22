cmake_minimum_required(VERSION 3.10)
project(Renderer)

# C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories("include/")

add_library(Core OBJECT
    "src/Dicom.cpp"
)

add_executable(mir "src/main.cpp" $<TARGET_OBJECTS:Core>)

set_property(TARGET mir PROPERTY CXX_STANDARD 17)

# Link DCMTK
if(WIN32)
    if(DEFINED ENV{DCMTK_HOME})
        message(STATUS "Found DCMTK_HOME: $ENV{DCMTK_HOME}")
    else()
    	message(FATAL_ERROR "Error: DCMTK_HOME not set!")
    endif()
    target_include_directories(mir PUBLIC "$ENV{DCMTK_HOME}/include")
    target_link_directories(mir PUBLIC "$ENV{DCMTK_HOME}/lib")
    target_link_libraries(mir "opengl32.lib" "ws2_32.lib" "wsock32.lib" "shlwapi.lib" "iphlpapi.lib" "netapi32.lib" "propsys.lib")
    target_link_libraries(mir PUBLIC "ofstd.lib" "oflog.lib" "dcmdata.lib" "dcmimgle.lib")
else()
    target_link_libraries(mir PUBLIC "libz.so" "libofstd.so" "liboflog.so" "libdcmdata.so" "libdcmimgle.so")
endif()

# Link <filesystem>
target_link_libraries(mir PUBLIC "stdc++fs")
