function main(target)
    -- 检查平台是否受支持
    local is_supported, error_msg = _check_platform()
    target:set("enabled", is_supported)
    if not is_supported then
        raise(error_msg)
    end

    -- 检查Fluent维度
    local FLUENT_DIM = get_config("FLUENT_DIM")
    _validate_fluent_dim(FLUENT_DIM)
    target:data_set("fluent_dim", FLUENT_DIM)

    -- 检查fluent实例是否存在
    local FLUENT_VERSION = get_config("FLUENT_VERSION")
    if not FLUENT_VERSION then
        cprint([[${yellow}Warning:${default}FLUENT_VERSION is not set. Better add "set_config("FLUENT_VERSION", "24.2.0")" to the root xmake.lua.]].."\n"..[[Guessing fluent path by checking environment variables...]])
    end

    -- 加载信息fluent实例相关信息
    import("load").set_fluent_info(target, FLUENT_VERSION)
    if target:data("fluent_path") == nil then
        return
    end

    -- 并行方式
    local PARALLEL_NODE = get_config("PARALLEL_NODE")
    _validate_fluent_parallel_node(PARALLEL_NODE)
    target:data_set("parallel_node", PARALLEL_NODE)

    -- 是否开启GPU
    local GPU_SUPPORT = get_config("GPU_SUPPORT")
    _validate_gpu_support(GPU_SUPPORT)
    target:data_set("gpu_support", GPU_SUPPORT)
end

function _check_platform()
    if is_plat("windows") then
        return true
    elseif is_plat("linux") then
        return false, "Linux is not supported yet"
    else
        return false, "Unsupported platform: only Windows is currently supported"
    end
end

function _validate_fluent_dim(value)
    local valid_dims = { ["2d"]=true, ["3d"]=true, ["2ddp"]=true, ["3ddp"]=true }
    if not valid_dims[value] then
        raise([[Please add a line like "set_config("FLUENT_DIM", "2ddp")" to root xmake.lua file to decide the solution type!
            Possible value are 2d, 3d, 2ddp or 3ddp ]])
    end
end

function _validate_fluent_parallel_node(value)
    local valid_pn = { ["none"]=true, ["smpi"]=true, ["vmpi"]=true, ["net"]=true, ["nmpi"]=true }
    if not valid_pn[value] then
        raise([[Please add a line like "set_config("PARALLEL_NODE", "smpi")" to root xmake.lua file to decide the parallel node!
            Possible value are none, smpi, vmpi, net, nmpi.
            none: a serial version of the solver
            smpi: parallel using shared memory (for multiprocessor machines)
            vmpi: parallel using shared memory or network with vendor MPI software
            net: parallel using network communicator with RSHD software
            nmpi: parallel using nmpi ]])
    end
end

function _validate_gpu_support(value)
    if type(value) ~= "boolean" then
        raise([[Please add a line like "set_config("GPU_SUPPORT", false)" to root xmake.lua file to decide whether to enable GPU support!]])
    end
end
