function get_fluent_arch()
    if os.is_host("windows") then
        if os.is_arch("x64") then
            fluent_arch = "win64"
        elseif os.is_arch("alpha") then
            fluent_arch = "ntalpha"
        elseif os.is_arch("x86") then
            fluent_arch = "ntx86"
        else
            raise("Error arch: "..os.host()..os.arch())
        end
    elseif os.is_host("linux") then
        if os.is_arch("x86_64") then
            fluent_arch = "lnamd64"
        else
            raise("Error arch: "..os.host()..os.arch())
        end
    end

    return fluent_arch
end

function find_fluent_dir(target, ansys_version)
    local version_table = (ansys_version):split('.', {plain = true})
    local envs = os.getenvs() -- 获取环境变量
    local fluent_path = path.join(envs["AWP_ROOT"..version_table[1]..version_table[2]], "fluent")

    if not os.exists(fluent_path) then
        raise("Cannot find FLUENT instance: ANSYS version is set as "..ansys_version.." now. Please check your configurations!")
    else
        -- cprint("${bright green}Find FLUENT instance in ${white}"..fluent_path)
    end

    return fluent_path
end

function _validate_fluent_dir(target, ansys_version)
    local version_table = (ansys_version):split('.', {plain = true})
    local envs = os.getenvs() -- 获取环境变量
    local fluent_path = path.join(envs["AWP_ROOT"..version_table[1]..version_table[2]], "fluent")

    if not os.exists(fluent_path) then
        fluent_path = nil
    end
    return fluent_path
end

function guess_fluent_version(target)
    local fluent_version = nil
    local majors = {24, 23, 22, 21, 20, 19, 18, 17}
    local minors = {2, 1}
    local fixs = {0}
    for _, major in ipairs(majors) do
        for _, minor in ipairs(minors) do
            for _, fix in ipairs(fixs) do
                fluent_version = major..'.'..minor..'.'..fix
                if nil ~= _validate_fluent_dir(target, fluent_version) then
                    goto FLUENT_HAVE_FOUND
                end
            end
        end
    end
    ::FLUENT_HAVE_FOUND::
    if fluent_version==nil then
        raise("Ansys Fluent is not found! Please check your envirenment variables.")
    else
        -- cprint([[${yellow}FLUENT_VERSION is not set explicitly. ]].."Fluent"..fluent_version..[[ is used.]])
        -- cprint([[${yellow}To set this explicitly, please add "set_config("FLUENT_VERSION", "21.2.0")" to the root xmake.lua]])
    end
    return fluent_version
end

function set_fluent_info(target, fluent_version)
    if fluent_version == nil then
        fluent_version = guess_fluent_version()
    end
    target:data_set("fluent_version", fluent_version)
    target:data_set("fluent_arch", get_fluent_arch())
    target:data_set("fluent_path", find_fluent_dir(target, fluent_version))
end

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