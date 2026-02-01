# 太阳系模拟程序

## 一、程序效果

该程序实现了一个简单的太阳系模拟效果，主要包括以下功能：

1. **太阳、地球和月球的模拟**：

   - 太阳固定在原点，进行缓慢的自转。
   - 地球围绕太阳进行缓慢的公转，同时有较快的自转。
   - 月球围绕地球进行较快的公转。
2. **坐标轴和地球轨道的显示**：

   - 程序绘制了三维空间中的坐标轴（X轴、Y轴、Z轴），以及地球的公转轨道，以便观察物体的相对位置。
3. **光照与纹理贴图**：

   - 使用了点光源模拟太阳光照，物体表面呈现出漫反射~~、镜面反射~~等光照效果。
   - 对星体进行了纹理贴图。
   - 通过shader来实现这些功能（resources/shaders/light_caster.vs&fs)
4. **摄像机控制**：

   - 用户可以通过鼠标和键盘自由移动摄像机，观察太阳系的不同角度。

   太阳、地球、月球的纹理贴图来取自 https://github.com/rubenandrebarreiro/3d-object-modelling-solar-system/

   采用了https://github.com/JoeyDeVries/LearnOpenGL对OpenGL所做的一些类封装（include/learnopengl），第三方依赖仍使用vcpkg安装。

---

## 二、使用方法

1. 配置说明：

   - 确保安装了vcpkg

     - git clone https://github.com/microsoft/vcpkg.git
       cd vcpkg; .\bootstrap-vcpkg.bat
     - 将CMakePresets.json中"VCPKG_ROOT": "D:/bin/vcpkg"改为实际的安装位置
   - 在22551013Weixianghan目录下执行

     - cmake --preset=graphics2025 		#会使用vcpkg自动安装所有必要的依赖
     - cmake --build build 				#在build\Debug目录生成可执行文件
     - ./build/Debug/solar_system.exe #运行程序
2. **操作说明**：

   - **摄像机移动**：
     - 按键 `W`、`A`、`S`、`D`、`J`、`K` 控制摄像机前后左右上下六个方向移动。
     - 鼠标移动控制视角方向。
   - **缩放**：
     - 使用鼠标滚轮缩放视野。
   - **退出程序**：
     - 按下 `ESC` 键退出程序。

---

## 三、主要实现方法

### 1. 地月模型变换矩阵计算

在程序中，地球和月球的运动通过模型变换矩阵来实现，主要包括以下几个步骤：

* **地球的公转与自转** ：

  * 地球的公转轨迹是一个圆形轨道，使用三角函数计算地球在轨道上的位置。
  * 地球的自转通过在模型矩阵中添加绕自身Y轴的旋转变换实现。
    ```c++
    float earthX = R_earthOrbit * cos(t * W_earthOrbit);
    float earthZ = R_earthOrbit * sin(t * W_earthOrbit);
    earthModel = glm::translate(earthModel, glm::vec3(earthX, 0.0f, earthZ));
    earthModel = glm::scale(earthModel, glm::vec3(Scale_earth));
    earthModel = glm::rotate(earthModel, t * earthSelfRotateAngleSpeed, glm::vec3(0, 1, 0));
    ```
* **月球的公转** ：
* 月球的运动是围绕地球的公转，使用类似的三角函数计算月球相对于地球的位移，然后加上地球对太阳(原点)的位移。

  ```cpp
  float moonX = R_moonOrbit * cos(t * W_moonOrbit);
  float moonZ = R_moonOrbit * sin(t * W_moonOrbit);
  moonModel = glm::translate(moonModel, glm::vec3(earthX + moonX, 0.0f, earthZ + moonZ));
  moonModel = glm::scale(moonModel, glm::vec3(Scale_moon));
  ```

通过这种方式，地球和月球的运动轨迹可以动态计算并实时更新，形成了地月系统的动态效果。

---

### 2. 光照计算

程序中使用了Phong光照模型来模拟太阳光对地球和月球的照射，光照计算分为以下几个部分：

* **环境光** ：

  * 模拟来自环境的均匀光照，使用光源的环境光强度与材质的漫反射贴图颜色相乘。
    ```cpp
    vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;
    ```
* **漫反射** ：

  * 通过光线方向与法线方向的点积计算漫反射强度，模拟光线照射到物体表面时的亮度变化。
    ```c
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;
    ```
* **镜面反射** ：

  * 计算视线方向与反射方向的点积，并取其幂次，模拟高光效果。在本项目中，没有使用镜面反射,但也实现在了shader中。
    ```c
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * texture(material.specular, TexCoords).rgb;

    ```
* **光照衰减** ：

  * 根据光源与物体之间的距离计算光照强度的衰减，增加真实感。
    ```c
    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    ```

---

### 3. 相机控制

程序中的相机控制基于 `learnopengl` 提供的 [Camera](vscode-file://vscode-app/c:/Users/25932/AppData/Local/Programs/Microsoft%20VS%20Code/resources/app/out/vs/code/electron-browser/workbench/workbench.html) 类，支持通过键盘自由移动和通过鼠标调整视角，其基于欧拉角，可能存在万向锁问题，因此通过offset规避了Yaw与Pitch相等的问题。

---

## 四、总结

该程序通过OpenGL实现了一个简单的太阳系模拟，展示了三维图形学中的基本技术，包括模型变换、光照计算、纹理贴图、和摄像机控制等。
