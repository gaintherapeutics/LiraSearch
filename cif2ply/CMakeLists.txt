cmake_minimum_required(VERSION 3.16)
project(cif2ply)

# CGAL and its components
find_package(CGAL QUIET COMPONENTS)

if (NOT CGAL_FOUND)

    message(STATUS "This project requires the CGAL library, and will not be compiled.")
    return()

endif()


# Boost and its components
find_package(Boost REQUIRED)

if (NOT Boost_FOUND)

    message(STATUS "This project requires the Boost library, and will not be compiled.")

    return()

endif()

set(CMAKE_CXX_STANDARD 14)

add_executable(cif2ply main.cpp include/file_parser.h include/skin_surface_writer.h include/skin_surface_processor.h)

add_to_cached_list(CGAL_EXECUTABLE_TARGETS cif2ply)

# Link the executable to CGAL and third-party libraries
target_link_libraries(cif2ply PRIVATE CGAL::CGAL)

target_include_directories(cif2ply PRIVATE ./include PRIVATE ./include/gemmi/include)