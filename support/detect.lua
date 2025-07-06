function main(target)
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
end