环境配置说明：

本项目使用vcpkg管理环境。需要通过以下步骤完成搭建

1.安装vcpkg

git clone https://github.com/microsoft/vcpkg.git
cd vcpkg; .\bootstrap-vcpkg.bat

2.将 CMakePretests.json中的"VCPKG_ROOT": "D:/vcpkg"改为上一步的安装位置

3.在22551013WeiXianghan目录下执行 
cmake --preset=graphics2025
cmake --build build
即可在build\Debug目录生成各个作业的可执行文件
例如运行:
./build/Debug/solar_system.exe

注意：路径中不能有中文字符