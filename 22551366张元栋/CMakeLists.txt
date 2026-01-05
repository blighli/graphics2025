cmake_minimum_required(VERSION 3.11)
project(opengl_learning VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

enable_testing()

set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

# # 创建着色器输出目录
# file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

# # 查找 GLSL 编译器
# find_program(GLSL_VALIDATOR glslangValidator)
# if(NOT GLSL_VALIDATOR)
#     message(FATAL_ERROR "glslangValidator not found! Install Vulkan SDK.")
# endif()

# # 自动查找所有着色器文件
# file(GLOB SHADER_SOURCES 
#     "${SHADER_SOURCE_DIR}/*.vert"
#     "${SHADER_SOURCE_DIR}/*.frag"
#     "${SHADER_SOURCE_DIR}/*.comp"
#     "${SHADER_SOURCE_DIR}/*.geom"
#     "${SHADER_SOURCE_DIR}/*.tesc"
#     "${SHADER_SOURCE_DIR}/*.tese"
# )

# # 为每个着色器创建编译规则
# foreach(shader_source ${SHADER_SOURCES})
#     # 获取文件名
#     get_filename_component(shader_name ${shader_source} NAME_WE)
    
#     # 设置输出文件路径
#     set(spv_output ${SHADER_BINARY_DIR}/${shader_name}.spv)
    
#     # 添加编译命令
#     add_custom_command(
#         OUTPUT ${spv_output}
#         COMMAND ${GLSL_VALIDATOR} -V ${shader_source} -o ${spv_output}
#         DEPENDS ${shader_source}
#         COMMENT "Compiling ${shader_name} -> ${shader_name}.spv"
#     )
    
#     list(APPEND COMPILED_SHADERS ${spv_output})
# endforeach()

# # 创建着色器编译目标
# add_custom_target(compile_shaders ALL DEPENDS ${COMPILED_SHADERS})
# add_custom_target(CopyAssets
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#             "${CMAKE_SOURCE_DIR}/models"
#             "${CMAKE_BINARY_DIR}/models"
#     COMMENT "Copying assets to build directory"
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#             "${CMAKE_SOURCE_DIR}/textures"
#             "${CMAKE_BINARY_DIR}/textures"
#     COMMENT "Copying assets to build directory"
# )
include_directories(third_party/glad/include)

add_subdirectory(third_party/glfw)
add_subdirectory(third_party/glm)



file(GLOB SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp) 
list(APPEND SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/src/glad.c)
add_executable(${PROJECT_NAME} ${SOURCE_PATH})

# add_dependencies(${PROJECT_NAME} compile_shaders)
# add_dependencies(${PROJECT_NAME} CopyAssets)
# target_include_directories (${PROJECT_NAME} PUBLIC
#     ${PROJECT_BINARY_DIR}
#     ${Vulkan_INCLUDE_DIRS}
# )
# target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)
target_link_libraries(${PROJECT_NAME} glfw)
target_link_libraries(${PROJECT_NAME} glm::glm)
# target_link_libraries(${PROJECT_NAME} tinyobjloader)

