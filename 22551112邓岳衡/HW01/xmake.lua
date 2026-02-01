add_rules("mode.debug", "mode.release")

set_languages("cxx17")

add_requires("glfw")
add_requires("glm")
add_requires("glad")
add_requires("opengl")

target("HW01")
    set_kind("binary")
    add_files("src/*.cpp")

    -- 添加远程依赖到目标
    add_packages("glfw", "glm", "glad", "opengl")
