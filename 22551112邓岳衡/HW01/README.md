
# 第一次作业

**快速开始**
- **项目文件**: 本仓库的构建脚本为 `xmake.lua`，源代码在 `src/`，可执行目标为 `HW01`。

**在 Windows (PowerShell) 上安装 xmake**
- **方法一（推荐，PowerShell 一行安装）:**

```powershell
iwr -useb https://xmake.io/shget.text | iex
```

- **方法二（通过 Scoop/Chocolatey）:**
- 使用 `scoop`:

```powershell
scoop install xmake
```

- 使用 `choco`:

```powershell
choco install xmake
```

- 安装校验:

```powershell
xmake --version
```

**前置依赖（Windows）**
- 需要安装 Visual C++ 编译工具（例如：Visual Studio 带有 C++ 工作负载或 Visual Studio Build Tools）。
- 需要联网以便 `xmake` 自动拉取 `xmake.lua` 中定义的第三方包（如 `glfw`、`glm`、`glad` 等）。

**使用 xmake 构建并运行项目**
1. 打开 PowerShell，进入项目根目录（包含 `xmake.lua` 的目录）：

```powershell
cd "...\3DAnimationCourse2025\22551112邓岳衡\HW01"
```

2. 构建（默认配置，xmake 会自动检测并安装 `add_requires` 的包）：

```powershell
xmake
```

3. 运行可执行文件（目标名为 `HW01`）：

```powershell
xmake run HW01
```

4. 若需切换到调试/发布模式，可先配置模式再构建：

```powershell
# 配置为 Debug 模式并构建
xmake f -m debug
xmake

# 或者 Release 模式
xmake f -m release
xmake
```

5. 清理构建产物：

```powershell
xmake clean
```

**关于源码与运行**
- 源文件: `src/main.cpp`。
- 程序会创建一个 GLFW 窗口并用 OpenGL 渲染一个带颜色插值的三角形。运行时请确保显卡驱动已正确安装并支持所请求的 OpenGL 版本。

**常见问题与排查**
- 如果 xmake 在拉取包时失败：确保能访问外网，或者手动安装所需包（常见方案：使用系统包管理器或从源码编译依赖）。
- 如果找不到编译器：请确认已安装 Visual C++ 工具链，并在 PowerShell 中能运行 `cl`（MSVC）或已正确设置环境变量。
- 如果 GLAD/GLFW 链接报错：查看 xmake 输出的依赖安装日志，必要时执行 `xmake -vD` 查看详细构建流程。
