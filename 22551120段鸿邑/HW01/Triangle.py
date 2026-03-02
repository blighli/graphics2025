from OpenGL.GL import *
from OpenGL.GLUT import *
from OpenGL.GLU import *

# 全局变量：控制视角的参数
rx = 0.0  # 绕X轴旋转角度
ry = 0.0  # 绕Y轴旋转角度
last_x = 0  # 鼠标上次X坐标
last_y = 0  # 鼠标上次Y坐标
mouse_down = False  # 鼠标左键是否按下
zoom = 1.0  # 缩放因子（控制相机距离）


def mouse_click(button, state, x, y):
    """处理鼠标点击事件"""
    global last_x, last_y, mouse_down
    if button == GLUT_LEFT_BUTTON:
        if state == GLUT_DOWN:
            mouse_down = True
            last_x = x
            last_y = y
        else:
            mouse_down = False


def mouse_motion(x, y):
    """处理鼠标拖动事件（旋转视角）"""
    global rx, ry, last_x, last_y
    if mouse_down:
        # 计算鼠标偏移量，调整旋转灵敏度（0.5为灵敏度系数）
        dx = x - last_x
        dy = y - last_y
        ry += dx * 0.5  # 绕Y轴旋转（水平拖动）
        rx += dy * 0.5  # 绕X轴旋转（垂直拖动）

        # 限制X轴旋转范围（避免视角翻转）
        rx = max(-90, min(90, rx))

        # 更新上次鼠标位置
        last_x = x
        last_y = y
    glutPostRedisplay()  # 标记窗口需要重新渲染


def mouse_wheel(button, dir, x, y):
    """处理鼠标滚轮事件（缩放视角）"""
    global zoom
    # 滚轮向上（放大）/向下（缩小），调整缩放因子
    if dir > 0:
        zoom *= 1.1
    else:
        zoom /= 1.1
    # 限制缩放范围（避免过度缩放）
    zoom = max(0.1, min(5.0, zoom))
    glutPostRedisplay()


def reshape(width, height):
    """窗口大小调整时更新投影矩阵"""
    glViewport(0, 0, width, height)  # 设置视口大小
    glMatrixMode(GL_PROJECTION)  # 切换到投影矩阵
    glLoadIdentity()  # 重置投影矩阵
    # 透视投影：60度视角，宽高比，近裁剪面0.1，远裁剪面100
    gluPerspective(60.0, width / height, 0.1, 100.0)
    glMatrixMode(GL_MODELVIEW)  # 切回模型视图矩阵
    glLoadIdentity()


def render():
    """核心渲染函数：绘制三角形并应用视角变换"""
    # 清空颜色缓冲区和深度缓冲区
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
    glLoadIdentity()  # 重置模型视图矩阵

    # 设置相机位置（看向原点）
    gluLookAt(
        0.0, 0.0, 5.0 * zoom,  # 相机位置（距离原点5*zoom单位）
        0.0, 0.0, 0.0,  # 观察目标（原点）
        0.0, 1.0, 0.0  # 相机上方向（Y轴）
    )

    # 应用旋转：先绕X轴，再绕Y轴
    glRotatef(rx, 1.0, 0.0, 0.0)
    glRotatef(ry, 0.0, 1.0, 0.0)

    # 绘制三角形面片（带颜色渐变）
    glBegin(GL_TRIANGLES)
    # 顶点1：顶部，红色
    glColor3f(1.0, 0.0, 0.0)
    glVertex3f(0.0, 1.0, 0.0)
    # 顶点2：左下，绿色
    glColor3f(0.0, 1.0, 0.0)
    glVertex3f(-1.0, -1.0, 0.0)
    # 顶点3：右下，蓝色
    glColor3f(0.0, 0.0, 1.0)
    glVertex3f(1.0, -1.0, 0.0)
    glEnd()

    glutSwapBuffers()  # 交换双缓冲区（避免画面闪烁）


def main():
    """程序主入口"""
    # 初始化GLUT
    glutInit()
    # 设置显示模式：双缓冲、RGB颜色、深度测试
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH)
    # 设置窗口大小
    glutInitWindowSize(800, 600)
    # 创建窗口
    glutCreateWindow(b"OpenGL Triangle (Mouse Control)")

    # 启用深度测试（避免物体重叠时渲染错误）
    glEnable(GL_DEPTH_TEST)
    # 设置背景色（深灰色）
    glClearColor(0.1, 0.1, 0.1, 1.0)

    # 注册回调函数
    glutDisplayFunc(render)  # 渲染回调
    glutReshapeFunc(reshape)  # 窗口大小调整回调
    glutMouseFunc(mouse_click)  # 鼠标点击回调
    glutMotionFunc(mouse_motion)  # 鼠标拖动回调
    glutMouseWheelFunc(mouse_wheel)  # 鼠标滚轮回调
    glutIdleFunc(render)  # 空闲时持续渲染

    # 启动GLUT主循环（阻塞，直到窗口关闭）
    glutMainLoop()


if __name__ == "__main__":
    main()