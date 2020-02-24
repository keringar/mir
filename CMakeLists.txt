cmake_minimum_required(VERSION 3.10)
project(Renderer)

# C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories("include/")

add_executable(mir "src/main.cpp" "src/Dicom.cpp")
set_property(TARGET mir PROPERTY CXX_STANDARD 17)

if (WIN32)
    set(DCMTK_DIR "$ENV{DCMTK_HOME}")
    target_compile_options(mir PUBLIC /MTd)
    target_link_options(mir PUBLIC /INCREMENTAL:NO /NODEFAULTLIB:MSVCRT)
endif()

# Link DCMTK
find_package(DCMTK NO_MODULE REQUIRED)
target_include_directories(mir SYSTEM PUBLIC ${DCMTK_INCLUDE_DIRS})
target_link_libraries(mir PUBLIC ${DCMTK_LIBRARIES})