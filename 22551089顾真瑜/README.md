## 浙江大学三维动画与交互技术2025冬
本项目基于OpenGL在Windows10下运行，依赖包括：
* glad
* glfw
* glm

### Lab1 绘制三角形
绘制了一个旋转的白色等边三角形
#### 环境配置
下载并安装glad和glfw，将`libglad.a`,`lbglfw3.a`和`libglfw3dll.a`放在`lib`目录下；将`glfw3.dll`放在`output`目录下（`main.exe`所在目录）

#### 运行
在Makefile目录下执行：
* `make run` 直接编译并运行
* `make`后运行`output`目录下的`main.exe`
