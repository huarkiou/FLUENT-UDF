add_rules("mode.release", "mode.debug")

set_config("FLUENT_VERSION", "24.1.0")
set_config("FLUENT_DIM", "3ddp")
set_config("GPU_SUPPORT", false)
set_config("PARALLEL_NODE", "smpi")

set_languages("cxx20")


includes("support") -- 将其中文件增加到xmake程序目录下rules文件夹中可不需要这行

target("utility")
    set_kind("headeronly")
    add_includedirs("utility", {public=true})
target_end()

typelist = {"node", "host"}

for i, type in ipairs(typelist) do
    target("libudf_"..type)
        add_rules("udf."..type)
        add_files("src/*.cpp")
        add_includedirs("src")
        add_deps("utility")
    target_end()
end

target("libudf")
    set_kind("phony")
    for i, type in ipairs(typelist) do
        add_deps("libudf_"..type)
    end
target()
