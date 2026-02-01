# 第四次作业

**快速开始**
- **项目类型**：CPU 软渲染器（Win32 窗口显示）+ 基础光照/多种着色与 Shadow Mapping。
- **可执行目标**：`rmRenderer`（源码入口：`renderer/main.cpp`）。
- **资源文件**：`assets/obj/`（模型 `*.obj` 与贴图 `*_diffuse.tga`、`*_nm.tga`、`*_spec.tga`）。
- **运行效果**：启动后会打开一个 `800×800` 的窗口，显示 `african_head` 模型的实时渲染结果（示例输出：`result.png`）。

**项目结构（关键文件）**
- `CMakeLists.txt`：CMake 构建脚本（会在编译后把 `assets/` 拷贝到可执行文件输出目录）。
- `CMakePresets.json`：Windows + MSVC（`cl.exe`）+ Ninja 的预设配置（x64/x86，Debug/Release）。
- `renderer/main.cpp`：程序入口（初始化窗口、相机、光源、Shader，并在循环中渲染）。
- `renderer/core/`：数学库、相机、FrameBuffer、模型加载（OBJ/TGA）、光栅化与渲染管线。
- `renderer/win32/`：Win32 平台层（创建窗口、输入回调、把 FrameBuffer 贴到窗口）。

**交互说明**
- 鼠标左键拖拽：Orbit（绕目标旋转相机）。
- 鼠标右键拖拽：Pan（平移相机/目标）。
- 鼠标滚轮：Dolly（缩放/推拉镜头）。
- `Space`：重置相机到默认位置。
- `Q`：开关阴影（Shadow Mapping + 简单 PCF）。
- `A`：绕 `Y` 轴旋转光源（改变光照方向/阴影方向）。
- `E`：切换 Shader（`BlinnPhone/NormalMap/Phone/Flat/Gouraud` 循环）。

## 编译步骤（Windows）

**前置工具**
- Visual Studio 2022（或 Visual Studio Build Tools）并安装 C++ 工作负载（确保 `cl.exe` 可用）。
- CMake（建议 3.20+，需支持 `CMakePresets.json`）。
- Ninja（推荐，用于与 presets 中的 `generator: Ninja` 匹配）。

**使用 CMake Presets 编译**
1. 打开 PowerShell，进入作业目录（包含 `CMakeLists.txt` 的目录）：

```powershell
cd "...\3DAnimationCourse2025\22551112邓岳衡\HW04"
```

2. 配置（以 x64 Debug 为例）：

```powershell
cmake --preset x64-debug
```

3. 编译：

```powershell
cmake --build out/build/x64-debug
```

4. 运行：

```powershell
.\out\build\x64-debug\rmRenderer.exe
```

> 说明：`CMakeLists.txt` 会在编译后自动把 `assets/` 拷贝到 `rmRenderer.exe` 同级目录下的 `assets/`，程序启动时会切换工作目录到该 `assets/` 并加载模型与贴图。

**Release 构建**

```powershell
cmake --preset x64-release
cmake --build out/build/x64-release
.\out\build\x64-release\rmRenderer.exe
```
