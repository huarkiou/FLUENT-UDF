set_config("FLUENT_VERSION", "21.2.0")
set_config("FLUENT_DIM", "2ddp")

local installdir ="$(buildir)/install"


includes("support") -- 将其中文件增加到xmake程序目录下rules文件夹中可不需要这行

target("libudf_node")
    add_rules("udf.node")
    add_files("src/*.cpp")
    add_includedirs("src")
    set_installdir(installdir)
target_end()

target("libudf_host")
    add_rules("udf.host")
    add_files("src/*.cpp")
    add_includedirs("src")
    set_installdir(installdir)
target_end()
