## 浙江大学三维动画与交互技术2025冬
本项目基于OpenGL在Windows10下运行，依赖包括：
* glad
* glfw
* glm

### Lab切换
Lab2中对实验代码结构进行了重构，现在不同场景通过切换Scene类来进行。切换场景需要修改`main.cpp: Line70`位置的定义。具体场景见`scene_func`目录下文件，目前包括：
* Lab1: `Scene_Triangle scene`
* Lab2: `Scene_SolarSystem scene`

### Lab1 绘制三角形
绘制了一个旋转的白色等边三角形
#### 环境配置
下载并安装glad和glfw，将`libglad.a`,`lbglfw3.a`和`libglfw3dll.a`放在`lib`目录下；将`glfw3.dll`放在`output`目录下（`main.exe`所在目录）

#### 运行
在Makefile目录下执行：
* `make run` 直接编译并运行
* `make`后运行`output`目录下的`main.exe`

### Lab2 太阳系
绘制了太阳、地球、月球的三星系统。
贴图源自：https://planetpixelemporium.com/planets.html
#### 基本功能
1. 三星进行自转，地球绕太阳旋转，月亮绕地球旋转。三星位于黄道平面。日月自转轴垂直于黄道平面，地球赤道平面与黄道平面夹角为23.5°
2. 太阳、月亮仅使用漫反射贴图，为地球额外添加了高光贴图
3. 使用PhongMaterial，具体材质设置及光照计算见`texture.h` `material.h/cpp` `light.h`以及 `shader/planetShader.vs/fs`。该实验中仅使用点光源作为太阳光源。未实现物体间光照遮挡关系。
