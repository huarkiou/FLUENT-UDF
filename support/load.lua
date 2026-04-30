-- 为target设置fluent实例相关信息
function set_fluent_info(target, fluent_version)
    if fluent_version == nil then
        fluent_version = _guess_fluent_version()
    end
    target:data_set("fluent_version", fluent_version)
    target:data_set("fluent_arch", _get_fluent_arch())
    target:data_set("fluent_path", _find_fluent_dir(target, fluent_version))
end

-- 为target添加fluent相关的头文件和链接库 （需先运行set_fluent_info获取fluent实例相关信息）
function add_fluent_headers_and_links(target)
    -- cprint("${bright green}Loading FLUENT headers and libraries ...")

    local solver_type = target:data("solver_type")
    local fluent_version = target:data("fluent_version")
    local fluent_arch = target:data("fluent_arch")
    local fluent_path = target:data("fluent_path")
    local gpu_support = target:data("gpu_support")
    local parallel_node = target:data("parallel_node")

    local version_table = (fluent_version):split('.', {plain = true})
    local fluent_lib_release = version_table[1]..version_table[2]..version_table[3]
    local fluent_release_path = path.join(fluent_path, "fluent"..fluent_version)

    local result = {}
    if solver_type:endswith("host") then
        local comm = "net"
        result.links = {"fl"..fluent_lib_release, "mport"}
        result.linkdirs = {
            path.join(fluent_release_path, fluent_arch, solver_type),
            path.join(fluent_release_path, "multiport", fluent_arch, comm, "shared")
        }
    elseif solver_type:endswith("node") then
        local comm = "mpi"
        if parallel_node ~= "none" and parallel_node ~= "net" then
            parallel_node = "mpi"
        end
        result.links = {"fl_"..parallel_node..fluent_lib_release, "mport"}
        result.linkdirs = {
            path.join(fluent_release_path, fluent_arch, solver_type),
            path.join(fluent_release_path, "multiport", fluent_arch, comm, "shared")
        }
    else
        result.links = {"fl"..fluent_lib_release}
        result.linkdirs = {
            path.join(fluent_release_path, fluent_arch, solver_type)
        }
    end

    result.includedirs = {
        path.join(fluent_release_path, fluent_arch, solver_type),
        path.join(fluent_release_path, "src", "main"),
        path.join(fluent_release_path, "src", "addon-wrapper"),
        path.join(fluent_release_path, "src", "io"),
        path.join(fluent_release_path, "src", "species"),
        path.join(fluent_release_path, "src", "pbns"),
        path.join(fluent_release_path, "src", "numerics"),
        path.join(fluent_release_path, "src", "sphysics"),
        path.join(fluent_release_path, "src", "storage"),
        path.join(fluent_release_path, "src", "mphase"),
        path.join(fluent_release_path, "src", "bc"),
        path.join(fluent_release_path, "src", "models"),
        path.join(fluent_release_path, "src", "material"),
        path.join(fluent_release_path, "src", "amg"),
        path.join(fluent_release_path, "src", "util"),
        path.join(fluent_release_path, "src", "mesh"),
        path.join(fluent_release_path, "src", "udf"),
        path.join(fluent_release_path, "src", "ht"),
        path.join(fluent_release_path, "src", "dx"),
        path.join(fluent_release_path, "src", "turbulence"),
        path.join(fluent_release_path, "src", "acoustics"),
        path.join(fluent_release_path, "src", "parallel"),
        path.join(fluent_release_path, "src", "etc"),
        path.join(fluent_release_path, "src", "ue"),
        path.join(fluent_release_path, "src", "dpm"),
        path.join(fluent_release_path, "src", "dbns"),
        path.join(fluent_release_path, "cortex", "src"),
        path.join(fluent_release_path, "client", "src"),
        path.join(fluent_release_path, "tgrid", "src"),
        path.join(fluent_release_path, "multiport", "src"),
        path.join(fluent_release_path, "multiport", "mpi_wrapper", "src"),
        -- path.join(fluent_release_path, "src", "archive"),
        -- path.join(fluent_release_path, "src", "sedm"),
        -- path.join(fluent_release_path, "realgas", "src"),
        path.join(fluent_path, "include")
        }

        if gpu_support then
            table.insert(result.includedirs, path.join(fluent_release_path, "multiport", "gpu_wrapper", "include"))
            table.insert(result.links, "OpenCL.lib")
            table.insert(result.linkdirs, path.join(fluent_release_path, "multiport", "gpu_wrapper", fluent_arch, "stub"))
        end

        target:add("sysincludedirs", result.includedirs)
        target:add("syslinks", result.links)
        target:add("linkdirs", result.linkdirs)
end

-- 设置fluent架构信息
function _get_fluent_arch()
    if os.is_host("windows") then
        if os.is_arch("x64", "x86_64") then
            fluent_arch = "win64"
        elseif os.is_arch("alpha") then
            fluent_arch = "ntalpha"
        elseif os.is_arch("x86") then
            fluent_arch = "ntx86"
        else
            raise("Unsupported Architecture: "..os.host()..os.arch())
        end
    elseif os.is_host("linux") then
        if os.is_arch("x86_64") then
            fluent_arch = "lnamd64"
        else
            raise("Unsupported Architecture: "..os.host()..os.arch())
        end
    else
        raise("Unsupported Host OS: "..os.host()..os.arch())
    end

    return fluent_arch
end

-- 查找fluent安装目录
function _find_fluent_dir(target, ansys_version)
    return _get_fluent_dir_by_version(target, _version_to_awp_root(ansys_version))
end

-- 根据环境变量获取指定版本的fluent安装目录
function _get_fluent_dir_by_version(target, key)
    local envs = os.getenvs() -- 获取环境变量
    local fluent_path = path.join(envs[key], "fluent")

    if not os.exists(fluent_path) then
        fluent_path = nil
    end
    return fluent_path
end

-- 获取最大版本的 AWP_ROOT 环境变量值
function _get_latest_awp_root()
    local max_ver = -1
    local max_key = nil
    local max_val = nil

    for name, value in pairs(os.getenvs()) do
        -- 匹配 AWP_ROOT 后紧跟数字的部分
        local suffix = name:match("^AWP_ROOT(%d+)$")
        if suffix then
            local ver = tonumber(suffix)
            if ver and ver > max_ver then
                max_ver = ver
                max_key = name
                max_val = value
            end
        end
    end

    if max_key then
        return max_key, max_val   -- 返回变量名和值，例如 "AWP_ROOT242" 和路径
    else
        return nil, nil
    end
end

-- 将AWP_ROOT242转换为版本号格式，例如 "24.2.0"
function _awp_root_to_version(value)
    local version = value:match("AWP_ROOT(%d+)")
    if version then
        return version:sub(1, 2) .. "." .. version:sub(3, 4) .. ".0"
    else
        return nil
    end
end

-- 将版本号转换为AWP_ROOT环境变量名，例如 "24.2.0" 转换为 "AWP_ROOT242"
function _version_to_awp_root(version)
    local major, minor = version:match("^(%d+)%.(%d+)%.%d+$")
    if major and minor then
        return "AWP_ROOT" .. major .. minor
    else
        return nil
    end
end

-- 从环境变量中选择最高版本的fluent
function _guess_fluent_version(target)
    local fluent_version = nil
    local key, value = _get_latest_awp_root()
    if key and value then
        cprint("Found FLUENT instance from environment variable ${bright green}"..key.."${white} : "..tostring(value))
        fluent_version = _awp_root_to_version(key)
    else
        goto FLUENT_NOT_FOUND
    end
    ::FLUENT_NOT_FOUND::
    if fluent_version == nil then
        cprint([[${yellow}Ansys Fluent is not found! Please check envirenment variables.]])
    end
    return fluent_version
end
