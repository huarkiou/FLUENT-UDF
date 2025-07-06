rule("udf.base")
    on_load(function (target)
        -- 检测系统环境及Fluent信息并设置相关变量
        import("detect")(target)
        -- 生成udf_names.c和ud_io1.h
        import("preprocess")(target)
        -- 添加预定义宏
        target:add("defines", "UDF_EXPORTING", "UDF_NT", string.upper(target:data("fluent_arch")))
    end)
    on_config(function (target)
        -- udf必须是动态链接库
        target:set("kind", "shared")
        -- 若没找到Fluent则禁用target
        if target:data("fluent_path") == nil then
            target:set("enabled", false)
            cprint([[${bright yellow}warning: ${clear}ANSYS FLUENT not found! Related targets disabled automatically.]])
        end
    end)
    before_build(function (target)
        config = import("core.project.config")
        cprint("Build target ${bright green}"..target:name()
            .."${default} for fluent instance ${bright green}"..target:data("fluent_path")
            .."${default} on ${bright green}"..config.plat().."-"..config.arch()
            .."${default} in ${bright green}"..config.mode().."${default} mode"
        )
    end)
    after_build(function (target)
        local output_dir = path.join(target:targetdir(), "libudf")
        import("install")(target, output_dir)
    end)
    on_install(function (target)
        if target:installdir() == nil then
            raise("Target install directory has not set yet! Example usage: xmake install -o D:/path/to/install libudf")
        end
        local output_dir = path.join(target:installdir(), "libudf")
        import("install")(target, output_dir)
    end)
rule_end()

rule("udf.host")
    add_deps("udf.base")
    add_orders("udf.base", "udf.host")
    on_config(function (target)
        target:data_set("solver_type", target:data("fluent_dim").."_host")
        import("load").add_fluent_headers_and_links(target)
    end)
rule_end()

rule("udf.node")
    add_deps("udf.base")
    add_orders("udf.base", "udf.node")
    on_config(function (target)
        target:data_set("solver_type", target:data("fluent_dim").."_node")
        import("load").add_fluent_headers_and_links(target)
    end)
rule_end()

rule("udf.seq")
    add_deps("udf.base")
    add_orders("udf.base", "udf.seq")
    on_load(function (target)
        cprint("${yellow}Deprecated rule \"udf.seq\". Please use \"udf.host\"/\"udf.node\" instead.${default}")
    end)
    on_config(function (target)
        target:data_set("solver_type", target:data("fluent_dim"))
        import("load").add_fluent_headers_and_links(target)
    end)
rule_end()
