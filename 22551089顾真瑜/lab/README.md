## 浙江大学三维动画与交互技术2025冬
本项目基于OpenGL在Windows10下运行，依赖包括：
* glad
* glfw
* glm
* assimp

### 环境配置
* 下载并安装glad和glfw，将`libglad.a`,`lbglfw3.a`和`libglfw3dll.a`放在`lib`目录下；将`glfw3.dll`放在`output`目录下（`main.exe`所在目录）
* 下载并安装assimp，将`libassimp.dll.a`放在`lib`目录下，将`libassimp-5.dll`放在`output`目录下

### Lab切换
Lab2中对实验代码结构进行了重构，现在不同场景通过切换Scene类来进行。切换场景需要修改`main.cpp: Line70`位置的定义。具体场景见`scene_func`目录下文件，目前包括：
* Lab1: `Scene_Triangle`
* Lab2: `Scene_SolarSystem`
* Lab3: `Scene_Nanosuit`

---

### Lab1 绘制三角形
绘制了一个旋转的白色等边三角形

#### 运行
在Makefile目录下执行：
* `make run` 直接编译并运行
* `make`后运行`output`目录下的`main.exe`

---

### Lab2 太阳系
绘制了太阳、地球、月球的三星系统。
贴图源自：https://planetpixelemporium.com/planets.html
#### 基本功能
1. 三星进行自转，地球绕太阳旋转，月亮绕地球旋转。三星位于黄道平面。日月自转轴垂直于黄道平面，地球赤道平面与黄道平面夹角为23.5°
2. 太阳、月亮仅使用漫反射贴图，为地球额外添加了高光贴图
3. 使用PhongMaterial，具体材质设置及光照计算见`texture.h` `material.h/cpp` `light.h`以及 `shader/planetShader.vs/fs`。该实验中仅使用点光源作为太阳光源。未实现物体间光照遮挡关系。

---

### Lab3 三维模型显示
对obj格式的模型提供了支持，使用了库`assimp`
支持点光源、直接光照以及聚光灯。
视角移动支持以模型为中心的平移旋转缩放及场景漫游。
模型来源：https://learnopengl-cn.github.io/data/nanosuit.rar

#### 基本功能
1. 加载了一个obj格式的模型
2. 点光源使用`lightShader.vs/fs`渲染为一个白色立方体，围绕模型旋转；直接光沿(-0.2, -1.0, -0.3)投射；聚光灯跟随相机，始终投射向正前方
3. 使用左键拖动屏幕以移动相机视角；使用右键拖动屏幕以围绕模型进行旋转；使用鼠标滚轮进行缩放；使用方向键进行相机位置移动
