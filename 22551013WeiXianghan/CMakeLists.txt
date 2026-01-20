cmake_minimum_required(VERSION 3.10)

project(graphics2025)

find_package(fmt CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

set(COMMON_LIBS fmt::fmt glfw glad::glad glm::glm assimp::assimp)

include_directories(${CMAKE_SOURCE_DIR}/include)

# Ensure the stb_image source file exists
file(WRITE ${CMAKE_SOURCE_DIR}/include/stb_image.cpp "#define STB_IMAGE_IMPLEMENTATION\n#include \"stb_image.h\"")

# Add stb_image.h implementation file
add_library(stb_image STATIC ${CMAKE_SOURCE_DIR}/include/stb_image.cpp)

# Add stb_image to COMMON_LIBS before creating executables
list(APPEND COMMON_LIBS stb_image)

macro(add_exe source)
    get_filename_component(target ${source} NAME_WE)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE ${COMMON_LIBS})
endmacro()


add_exe(homework_1/colored_triangle.cpp)
add_exe(homework_2/solar_system.cpp)
add_exe(homework_3/model_loading.cpp)
add_exe(final_project/car_game.cpp)

# Generate root_directory.h dynamically
configure_file(${CMAKE_SOURCE_DIR}/include/root_directory.h.in ${CMAKE_SOURCE_DIR}/include/root_directory.h @ONLY)

# Enable _CRT_SECURE_NO_WARNINGS to suppress warnings on Windows
if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# As final project import big model, set Release as default build type
# if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
#     set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
#     message(STATUS "CMAKE_BUILD_TYPE not set. Defaulting to Release.")
# endif()





