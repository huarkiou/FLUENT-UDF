function main(target, base_dir)
        if base_dir == nil then
            cprint("${red}Wrong output directory: " .. (base_dir or "nil") .. "${default}")
            return
        end
        local output_dir = path.join(base_dir, target:data("fluent_arch"), target:data("solver_type"))
        -- 复制生成的二进制目标
        os.cp(path.join(target:targetdir(), target:name()..".dll"), path.join(output_dir, "libudf.dll"))
        os.trycp(path.join(target:targetdir(), target:name()..".pdb"), path.join(output_dir, "libudf.pdb"))
        -- 复制生成二进制目标所用的部分源码
        for _, sourcebatch in pairs(target:sourcebatches()) do
            local sourcekind = sourcebatch.sourcekind
            for _, sourcefile in ipairs(sourcebatch.sourcefiles) do
                os.cp(sourcefile, path.join(base_dir, "src", sourcefile))
            end
        end
        -- 写入编译信息
        local config = import("core.project.config")
        local buildinfo = format("Build Time: %s\n"..
                                "\nFluent Path: %s\n"..
                                " Solver Type: %s\n"..
                                " Fluent Version: %s\n"..
                                " Fluent Arch: %s\n"..
                                " GPU Support: %s\n"..
                                " Parellel Node: %s\n"..
                                "\nBuild configurations are below: \n"..
                                " Library Kind: %s\n"..
                                " Target Arch: %s\n"..
                                " Build Mode: %s\n",
                                os.date("%Y-%m-%d %H:%M:%S", os.time()),
                                target:data("fluent_path"),
                                target:data("solver_type"),
                                target:data("fluent_version"),
                                target:data("fluent_arch"),
                                target:data("gpu_support"),
                                target:data("parallel_node"),
                                target:kind(), config.plat().."-"..config.arch(),
                                config.mode()
            )
        io.writefile(path.join(output_dir, "buildinfo.txt"), buildinfo)
end