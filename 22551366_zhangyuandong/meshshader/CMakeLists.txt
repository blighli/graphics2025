cmake_minimum_required(VERSION 3.22...3.31)
project(vk_mini_samples VERSION 2.0 LANGUAGES C CXX)

# Add the cmake folder to the module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(NvproCore2 REQUIRED)

set(COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/common")

# Add common library first as other samples might depend on it
add_subdirectory(common)


add_subdirectory(mesh_shader)




