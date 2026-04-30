-- 根据当前平台生成可执行文件的后缀
local function _exe_suffix()
    return os.is_host("windows") and ".exe" or ""
end

-- 获取源文件列表并去重
function _filter_sourcefiles(target)
    local seen = {}
    local sourcefiles = {}
    for _, sourcebatch in pairs(target:sourcebatches()) do
        for _, sourcefile in ipairs(sourcebatch.sourcefiles) do
            local name = path.filename(sourcefile)
            if name ~= "udf_names.c" and not seen[sourcefile] then
                seen[sourcefile] = true
                table.insert(sourcefiles, sourcefile)
            end
        end
    end
    return sourcefiles
end

-- 使用sed命令提取UDF函数的声明和数据
function _run_sed(sed_path, pattern, sourcefile)
    local cmd = string.format('%q -n %s %q', sed_path, pattern, sourcefile)
    local out, err = os.iorun(cmd)
    if out==nil then
        raise("sed failed on %s: %s",sourcefile, err or "unknown error")
    end
    return out
end

-- 生成udf_names.c文件，包含所有UDF函数的声明和一个UDF_Data数组
function _generate_udfnames(sourcefiles, tools_path, gen_dir)
    local sed_path = path.join(tools_path, "sed".._exe_suffix())
    local pattern_decl = [[ "s/^.*\(\<DEFINE_[_A-Z]*([, _a-zA-Z0-9]*)\).*$/EXTERN_C \1;/p" ]]
    local pattern_data = [[ "s/^.*\<DEFINE_\([_A-Z]*\)( *\([_a-zA-Z0-9]*\)[, _a-zA-Z0-9]*).*$/    \{\"\2\", (void (*)(void))\2, UDF_TYPE_\1\},/p" ]]

    local parts = {[==[
/* This file generated automatically. */
/*          Do not modify.            */
#include "udf.h"
#include "prop.h"
#include "dpm.h"

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

]==]}
    for _,sourcefile in ipairs(sourcefiles) do
        -- if not path.filename(sourcefile):startswith("udf_names.c") then
        local fullpath = path.join("$(projectdir)", sourcefile)
        local out = _run_sed(sed_path, pattern_decl, fullpath)
        table.insert(parts, out)
        -- end
    end
    table.insert(parts, "\nEXPORT UDF_Data udf_data[] = {\n")

    for _,sourcefile in ipairs(sourcefiles) do
        if not path.filename(sourcefile):startswith("udf_names.c") then
            local fullpath = path.join("$(projectdir)", sourcefile)
            local out = _run_sed(sed_path, pattern_data, fullpath)
            table.insert(parts, out)
        end
    end
    table.insert(parts, [==[
};
EXPORT int n_udf_data = sizeof(udf_data)/sizeof(UDF_Data);

#include "version.h"
EXPORT void UDF_Inquire_Release(int *major, int *minor, int *revision)
{
    *major = RampantReleaseMajor;
    *minor = RampantReleaseMinor;
    *revision = RampantReleaseRevision;
}
]==])
    local udf_names_str = table.concat(parts)
    io.writefile(path.join(gen_dir, "udf_names.c"), udf_names_str)
end

function _generate_udfio(sourcefiles, tools_path, gen_dir)
    local resolve = path.join(tools_path, "resolve" .. _exe_suffix())
    local filelist_parts = {}
    for _, sourcefile in ipairs(sourcefiles) do
        table.insert(filelist_parts, string.format('%q', path.join("$(projectdir)", sourcefile)))
    end
    local filelist = table.concat(filelist_parts, " ")

    local out_file = path.join(gen_dir, "ud_io1.h")
    local command = string.format('%q -udf %s -head_file %q', resolve, filelist, out_file)
    local ok, err = os.run(command)   -- 检查返回值
    if not ok then
        cprint("resolve failed: %s", err or "unknown error")
    end
end

function main(target)
    local autogendir = target:autogendir()

    local sourcefiles = _filter_sourcefiles(target)
    local max_src_mtime = 0
    for _, sourcefile in ipairs(sourcefiles) do
        local mtime = os.mtime(sourcefile) or 0
        if mtime > max_src_mtime then
            max_src_mtime = mtime
        end
    end

    local gen_mtime = os.mtime(path.join(autogendir, "udf_names.c")) or 0
    if max_src_mtime > gen_mtime then
        local fluent_path = target:data("fluent_path")
        local tools_path = os.is_host("windows") and path.join(fluent_path, "ntbin/win64") or path.join(fluent_path, "bin")
        _generate_udfnames(sourcefiles, tools_path, autogendir)
        _generate_udfio(sourcefiles, tools_path, autogendir)
    end

    target:add("files", path.join(autogendir, "udf_names.c"))
    target:add("includedirs", autogendir)

    return
end
