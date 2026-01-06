cmake_minimum_required(VERSION 3.10)

project(graphics2025)

find_package(fmt CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

set(COMMON_LIBS fmt::fmt glfw glad::glad glm::glm assimp::assimp)


include_directories(${CMAKE_SOURCE_DIR}/include)

macro(add_exe source)
    get_filename_component(target ${source} NAME_WE)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE ${COMMON_LIBS})
endmacro()


add_exe(homework_1/colored_triangle.cpp)
add_exe(homework_2/solar_system.cpp)

# Add stb_image.h implementation file
add_library(stb_image STATIC ${CMAKE_SOURCE_DIR}/include/stb_image.cpp)

# Ensure the source file exists
file(WRITE ${CMAKE_SOURCE_DIR}/include/stb_image.cpp "#define STB_IMAGE_IMPLEMENTATION\n#include \"stb_image.h\"")

# Link stb_image to the solar_system target
target_link_libraries(solar_system PRIVATE stb_image)

# Add stb_image to COMMON_LIBS for automatic linking
list(APPEND COMMON_LIBS stb_image)

# Generate root_directory.h dynamically
configure_file(${CMAKE_SOURCE_DIR}/include/root_directory.h.in ${CMAKE_SOURCE_DIR}/include/root_directory.h @ONLY)





