环境配置说明：

本项目使用vcpkg管理环境。需要通过以下步骤完成搭建

1.安装vcpkg。例如在D:/bin下执行：
```
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg; .\bootstrap-vcpkg.bat
```
如果安装在其他目录，请在CMakePretests.json中修改VCPKG_ROOT

2.将 CMakePretests.json中的"generator": "Visual Studio 18 2026" 改为您电脑上安装的Visual Studio版本，例如Visual Studio 17 2022 或者 Visual Studio 16 2019。

3.在22551013WeiXianghan目录下执行 
```
cmake --preset=graphics2025
cmake --build build
```
即可在build/Debug目录生成各个作业的可执行文件


然后，在22551013WeiXianghan目录下执行：

运行作业1
```
./build/Debug/colored_triangle.exe
```

运行作业2
```
./build/Debug/solar_system.exe
```

运行作业3
```
./build/Debug/model_loading.exe
```

运行final_project
```
./build/Debug/car_game.exe
```


注意：路径中不能有中文字符