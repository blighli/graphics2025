环境配置说明：

本项目使用vcpkg管理环境。需要通过以下步骤完成搭建

1.安装vcpkg

git clone https://github.com/microsoft/vcpkg.git
cd vcpkg; .\bootstrap-vcpkg.bat

2.将 CMakePretests.json中的"VCPKG_ROOT": "D:/vcpkg"改为实际的安装位置

3.执行 
cmake --preset=graphics2025
cmake --build build
在build\Debug目录生成可执行文件

注意：路径中不要有中文字符