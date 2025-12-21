cmake_minimum_required(VERSION 3.26) # 指定的 CMake 最小版本

project(MyProject) # 项目名称，可以更改为您想要的名称

set(CMAKE_CXX_STANDARD 11) # 设置 C++ 标准，这里使用的是 C++11，可以根据需要修改，例如使用 C++14/C++17/C++20

include_directories(include)

# 添加可执行文件
add_executable(MyExecutable main.cpp) # 'MyExecutable' 是输出的可执行文件名
