function _filter_sourcefiles(target)
    -- 获取源文件列表并去重
    -- cprintf("${bright green}Filter sourcefiles:")
    local sourcefiles = {}
    for _, sourcebatch in pairs(target:sourcebatches()) do
        for _, sourcefile in ipairs(sourcebatch.sourcefiles) do
            if not path.filename(sourcefile):startswith("udf_names.c") and not table.contains(sourcefiles, sourcefile) then
                table.insert(sourcefiles, sourcefile)
                -- cprintf("${white} "..sourcefile)
            end
        end
    end
    return sourcefiles
end

function _generate_udfnames(sourcefiles, tools_path, gen_dir)
    -- cprintf("${bright green}Generate ${white}udf_names.c ")

    local udf_names_str = [==[
/* This file generated automatically. */
/*          Do not modify.            */

#include "udf.h"
#include "prop.h"
#include "dpm.h"

]==]

    local tmp_file = os.tmpfile()
    local sed_path = path.join(tools_path, "sed"..(os.is_host("windows") and ".exe" or ""))
    local sed_cmd = '"'..sed_path..'"'..' -n '
    sedpattern1 = [[ "s/^.*\(\<DEFINE_[_A-Z]*([, _a-zA-Z0-9]*)\).*$/EXTERN_C \1;/p" ]]
    sedpattern2 = [[ "s/^.*\<DEFINE_\([_A-Z]*\)( *\([_a-zA-Z0-9]*\)[, _a-zA-Z0-9]*).*$/    \{\"\2\", (void (*)(void))\2, UDF_TYPE_\1\},/p" ]]

    local premake_cmd_path = path.join(gen_dir, "_premake"..(os.is_host("windows") and ".cmd" or ".sh"))
    for _, sourcefile in ipairs(sourcefiles) do
        if path.filename(sourcefile):startswith("udf_names.c") then
            goto continue
        end
        local command = sed_cmd..sedpattern1..' "'..path.join("$(projectdir)", sourcefile)..'"'
        io.printf(premake_cmd_path, command.." > "..tmp_file)
        os.run(premake_cmd_path)
        local outdata = io.readfile(tmp_file)
        io.writefile(tmp_file, " ")
        udf_names_str = udf_names_str..outdata
        ::continue::
    end

    udf_names_str = udf_names_str.."\n__declspec(dllexport) UDF_Data udf_data[] = {\n"

    for _, sourcefile in ipairs(sourcefiles) do
        if path.filename(sourcefile):startswith("udf_names.c") then
            goto continue
        end
        local command = sed_cmd..sedpattern2..' "'..path.join("$(projectdir)", sourcefile)..'"'
        io.printf(premake_cmd_path, command.." > "..tmp_file)
        os.run(path.join(premake_cmd_path))
        local outdata = io.readfile(tmp_file)
        io.writefile(tmp_file, " ")
        udf_names_str = udf_names_str..outdata
        ::continue::
    end
    os.rm(premake_cmd_path)
    os.rm(tmp_file)
    udf_names_str = udf_names_str..[==[
};
__declspec(dllexport) int n_udf_data = sizeof(udf_data)/sizeof(UDF_Data);

#include "version.h"

__declspec(dllexport) void UDF_Inquire_Release(int *major, int *minor, int *revision)
{
    *major = RampantReleaseMajor;
    *minor = RampantReleaseMinor;
    *revision = RampantReleaseRevision;
}
]==]
    io.print(path.join(gen_dir, "udf_names.c"), udf_names_str)
end

function _generate_udfio(sourcefiles, tools_path, gen_dir)
    -- cprint("${bright green}Generate ${white}udf_io1.h ")
    local udf_io_str = ""
    local resolve_cmd = '"'..path.join(tools_path, "resolve"..(os.is_host("windows") and ".exe" or ""))..'"'
    resolve_cmd = resolve_cmd.." -udf "
    local filelist = ""
    for _, sourcefile in ipairs(sourcefiles) do
        filelist = filelist.." "..path.join("$(projectdir)", sourcefile)
    end
    local command = resolve_cmd..filelist.." -head_file ".. path.join(gen_dir, "ud_io1.h")
    os.run(command)
end

function main(target)
    local autogendir = target:autogendir()

    -- 如果源代码时间比生成的代码新才进行处理，否则直接返回
    udfnamefile = path.join(autogendir, "udf_names.c")
    for _, sourcebatch in pairs(target:sourcebatches()) do
        for _, sourcefile in ipairs(sourcebatch.sourcefiles) do
            if not path.filename(sourcefile):startswith("udf_names.c") then
                genmtime = os.mtime(udfnamefile) -- 似乎这个os.mtime只能识别他自己生成的文件修改时间，有bug好像始终返回0
                srcmtime = os.mtime(sourcefile)
                -- print(srcmtime.." : "..genmtime)
                if srcmtime >= genmtime then
                    local fluent_path = target:data("fluent_path")
                    local sourcefiles = _filter_sourcefiles(target)
                    local tools_path
                    if os.is_host("windows") then
                        tools_path = path.join(fluent_path, "ntbin/win64")
                    else
                        tools_path = path.join(fluent_path, "bin")
                    end
                    _generate_udfnames(sourcefiles, tools_path, autogendir)
                    _generate_udfio(sourcefiles, tools_path, autogendir)
                    break
                end
            end
        end
    end

    target:add("files", path.join(autogendir, "udf_names.c"))
    target:add("includedirs", autogendir)

    return
end
