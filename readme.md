# FLUENT UDF

## 介绍

- 简介：使用xmake构建FLUENT UDF

- 拟解决的问题：FLUENT内置的编译脚本有点坑，例如:
    1. DEFINE_XX宏那一行不能有多余字符，导致我习惯的左大括号放在上一行不换行会导致生成udf_names.c出错；
    2. 在部分版本的FLUENT上内置编译脚本匹配DEFINE_XX宏时有时会漏掉部分内容；
    3. 当系统语言是中文时，用FLUENT内的编译udf，如果代码有错误，编译报错的提示在FLUENT的console中会乱码，而且信息多且乱；
    4. 编译器开启的语言标准采用默认，现在想开c++20标准还得去改他的编译脚本；
    5. 调用第三方库麻烦。

本项目的核心方案是：**利用 FLUENT 安装目录自带的 `sed` 和 `resolve` 工具，在构建阶段自动从源代码中提取所有 DEFINE_XX 宏并生成 `udf_names.c` 和 `ud_io1.h`**，从而绕过 FLUENT 内置编译脚本的局限。

## 说明

本脚本通过```AWP_ROOT<ver>```环境变量寻找FLUENT的安装位置，需要提前设置环境变量。

（例如，环境变量```AWP_ROOT241```中指定ANSYS2024R1的安装目录```"C:\Program Files\ANSYS2024R1\v241"```）

一般Windows上正常安装完成后ANSYS的安装程序已经自动生成了一系列的环境变量，不需要用户手动设置了。

## 配置

在 `xmake.lua` 中通过 `set_config` 设置以下选项：

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `FLUENT_VERSION` | 自动检测（取最高版本） | 指定 Fluent 版本，如 `"24.2.0"` |
| `FLUENT_DIM` | `"2ddp"` | 求解维度：`"2d"` / `"3d"` / `"2ddp"` / `"3ddp"` |
| `PARALLEL_NODE` | `"smpi"` | 并行方式：`"none"` / `"smpi"` / `"vmpi"` / `"net"` / `"nmpi"` |
| `GPU_SUPPORT` | `false` | 是否启用 GPU 支持 |

xmake.lua 已启用 C++20 标准（`set_languages("cxx20")`），并配置了 C++20 模块支持（`src/udf.cppm` 为示例），但因 clangd 等工具链对模块支持尚不完善，暂不建议在生产代码中实际使用。

## 安装步骤

### Windows

1. 安装C/C++环境 (**已安装则跳过**) (*推荐Visual Studio Community 2022，安装时工作负载勾选"使用C++的桌面开发"*)

2. 安装ANSYS Fluent (**已安装则跳过**) (*测试过2020R1~2024R2，更低的理论上也行但是没试过*)

3. 安装[xmake](https://github.com/xmake-io/xmake/releases/)（需要 >= 3.0.0），确保已将其添加至环境变量

4. 下载本仓库内容
    ```sh
    git clone https://github.com/huarkiou/FLUENT-UDF.git
    cd FLUENT-UDF # 进入新下载的目录
    ```

5. 根据需要修改目录FLUENT-UDF/src下的udf代码和FLUENT-UDF/xmake.lua中的内容（建议先清空 `src/` 中已有的测试文件，再放入自己的 UDF 代码）

6. 编译UDF生成libudf
    ```sh
    xmake build libudf # 编译生成UDF动态链接库
    ```
7. 将编译好的libudf安装到目标路径
    ```sh
    xmake install -a -o D:/path/to/fluent/casedir
    ```
   安装后目录结构如下：
   ```
   D:/path/to/fluent/casedir/libudf/
   ├── win64/
   │   ├── 2ddp_host/
   │   │   └── libudf.dll
   │   └── 2ddp_node/
   │       └── libudf.dll
   └── src/   # UDF 源码备份
   ```

8. 在FLUENT中导入libudf：**User Defined → Functions → Compiled**，Library Name 填入 `libudf`，将 Compiled UDFs 目录指向安装路径，点击 Load。

### Linux

- 暂不直接支持。`support/load.lua` 已含 Linux 架构检测逻辑，但 `detect.lua` 中暂设为不可用，需要安装了 Fluent 的 Linux 测试环境进行适配验证。

## 推荐环境

vscode + clangd + xmake

构建后 `build/` 目录下自动生成 `compile_commands.json`，clangd 可直接使用，无需额外配置。

Windows下调试可以用命令```xmake project -k cmake```生成CMakeLists.txt或者```xmake project -k vsxmake```生成vs的sln项目，然后用VS附加到进程调试更方便。

## 基本示例

基本用法见 `examples/`，更多实际应用示例见 `apps/`（编译时需将对应文件复制到 `src/`）。

## udfwarpper（可选）

`udfwarpper/` 是一个对 FLUENT UDF 原语的简易 C++20 封装：

- `udf::print` / `udf::println` / `udf::info` / `udf::error` — 格式化输出，替代 `Message`
- `udf::host_to_node_data` / `udf::node_to_host_data` — Host/Node 间传输 `std::vector` 和 `std::string`
- `util::deg2rad` / `util::rad2deg`、字符串工具等

如不需要，可在 `xmake.lua` 中删除 `add_deps("udfwarpper")` 及对应的 `target("udfwarpper")` 定义。

## TODO

- [ ] Linux 平台支持（检测逻辑已部分就位，但未完成适配）
- [ ] 完善 `examples/` 中的示例文档和用例
