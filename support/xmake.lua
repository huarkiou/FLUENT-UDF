rule("udf.base")
    on_load(function (target)
        if not is_plat("windows", "linux") then
            target:set("enabled", false)
            if is_plat("linux") then
                raise("Linux is not supported yet")
            end
        end

        -- 检查Fluent维度
        local FLUENT_DIM = get_config("FLUENT_DIM")
        if FLUENT_DIM == nil then
            raise([[Please add a line like "set_config("FLUENT_DIM", "2ddp")" to root xmake.lua file to decide the solution type!
             Possible value are 2d, 3d, 2ddp or 3ddp ]])
        else
            target:data_set("fluent_dim", FLUENT_DIM)
        end

        -- 检查fluent实例是否存在并设置所需变量
        local FLUENT_VERSION = get_config("FLUENT_VERSION")
        import("load").set_fluent_info(target, FLUENT_VERSION)
        if target:data("fluent_path") == nil then
            return
        end

        -- 并行方式
        local PARALLEL_NODE = get_config("PARALLEL_NODE")
        if PARALLEL_NODE == nil then
            raise([[Please add a line like "set_config("PARALLEL_NODE", "smpi")" to root xmake.lua file to decide the parallel node!
             Possible value are none, smpi, vmpi, net, nmpi.
                none: a serial version of the solver
                smpi: parallel using shared memory (for multiprocessor machines)
                vmpi: parallel using shared memory or network with vendor MPI software
                net: parallel using network communicator with RSHD software
                nmpi: parallel using nmpi ]])
        end
        target:data_set("parallel_node", PARALLEL_NODE)

        -- 是否开启GPU
        local GPU_SUPPORT = get_config("GPU_SUPPORT")
        if GPU_SUPPORT == nil then
            GPU_SUPPORT = false
        end
        target:data_set("gpu_support", GPU_SUPPORT)

        -- 生成udf_names.c和ud_io1.h
        import("preprocess")(target)

        -- 添加预定义宏
        target:add("defines", "UDF_EXPORTING", "UDF_NT", string.upper(target:data("fluent_arch")))
    end)
    on_config(function (target)
        -- udf是动态链接库
        target:set("kind", "shared")

        if target:data("fluent_path") == nil then
            target:set("enabled", false)
            cprint([[${bright yellow}warning: ${clear}ANSYS FLUENT not found! Related targets disabled automatically.]])
        end
    end)
    before_build(function (target)
        if target:data("fluent_path") == nil then
            raise("ANSYS FLUENT not found!")
        else
            config = import("core.project.config")
            cprint("Build target ${bright green}"..target:name()
                .."${default} for fluent instance ${bright green}"..target:data("fluent_path")
                .."${default} on ${bright green}"..config.plat().."-"..config.arch()
                .."${default} in ${bright green}"..config.mode().."${default} mode"
            )
        end
    end)
    after_build(function (target)
        local output_dir = path.join(target:targetdir(), "libudf", target:data("fluent_arch"), target:data("solver_type"))
        os.cp(path.join(target:targetdir(), target:name()..".dll"), path.join(output_dir, "libudf.dll"))
        os.trycp(path.join(target:targetdir(), target:name()..".pdb"), path.join(output_dir, "libudf.pdb"))
        for _, sourcebatch in pairs(target:sourcebatches()) do
            local sourcekind = sourcebatch.sourcekind
            for _, sourcefile in ipairs(sourcebatch.sourcefiles) do
                os.cp(sourcefile, path.join(target:targetdir(), "libudf", "src", sourcefile))
            end
        end
    end)
    on_install(function (target)
        if target:installdir() == nil then
            raise("Target install directory has not set yet! example usage: xmake install -a -o D:/path/to/install libudf")
        end
        local output_dir = path.join(target:installdir(), "libudf", target:data("fluent_arch"), target:data("solver_type"))
        os.cp(path.join(target:targetdir(), target:name()..".dll"), path.join(output_dir, "libudf.dll"))
        os.trycp(path.join(target:targetdir(), target:name()..".pdb"), path.join(output_dir, "libudf.pdb"))
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
    on_config(function (target)
        target:data_set("solver_type", target:data("fluent_dim"))
        import("load").add_fluent_headers_and_links(target)
    end)
rule_end()
