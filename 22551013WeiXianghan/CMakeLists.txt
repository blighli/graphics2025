cmake_minimum_required(VERSION 3.10)

project(graphics2025)

find_package(fmt CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

set(COMMON_LIBS fmt::fmt glfw glad::glad)


include_directories(${CMAKE_SOURCE_DIR}/include)

macro(add_exe source)
    get_filename_component(target ${source} NAME_WE)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE ${COMMON_LIBS})
endmacro()


add_exe(homework_1/hello_triangle.cpp)





