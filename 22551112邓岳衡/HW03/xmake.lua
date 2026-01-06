add_rules("mode.debug", "mode.release")
add_defines("VK_RESULT_THROW")

set_languages("c++20")

add_requires("glfw")
add_requires("stb")
add_requires("spdlog")
add_requires("assimp")

target("LearnVulkan")
    set_kind("binary")
    set_default(true)
    add_files("src/**.cpp")
    add_packages("glfw")
    add_packages("stb")
    add_packages("spdlog")
    add_packages("assimp")

    add_includedirs("include", {public = true})

    -- Vulkan SDK
    add_includedirs("$(env VULKAN_SDK)/include", {public = true})
    add_linkdirs("$(env VULKAN_SDK)/lib", {public = true})
    add_links("vulkan-1", {public = true})

    if is_mode("debug") then
        set_rundir("$(projectdir)")
    end