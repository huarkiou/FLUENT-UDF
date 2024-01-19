# FLUENT UDF

## 介绍

- 简介：使用xmake构建FLUENT UDF

- 拟解决的问题：FLUENT内置的编译脚本有点坑，例如:
    1. DEFINE_XX宏那一行不能有多余字符，导致我习惯的左大括号放在上一行不换行会导致生成udf_names.c出错；
    2. 当系统语言是中文时，用FLUENT内的编译udf，如果代码有错误，编译报错的提示在FLUENT的console中会乱码，而且信息多且乱；
    3. 编译器开启的语言标准采用默认，现在想开c++20标准还得去改他的编译脚本；
    4. 调用第三方库麻烦；
    5. 最开始配置UDF的编译环境有很多小坑，虽然很好解决，但是令人不爽。

## 安装步骤

### Windows 10/11

1. 安装C/C++环境 (**已安装则跳过**) (*推荐Visual Studio Community 2022，安装时工作负载勾选"使用C++的桌面开发"*)

2. 安装ANSYS Fluent (**已安装则跳过**) (*测试过2020R1~2024R1，更低的理论上也行但是没试过*)

3. 安装[xmake](https://github.com/xmake-io/xmake/releases/)，确保已将其添加至环境变量

4. 下载本仓库内容
    ```sh
    git clone https://github.com/huarkiou/FLUENT-UDF.git
    cd FLUENT-UDF # 进入新下载的目录
    ```

5. 根据需要修改目录FLUENT-UDF/src下的udf代码和FLUENT-UDF/xmake.lua中的内容

6. 编译UDF生成libudf
    ```sh
    xmake build libudf # 编译生成UDF动态链接库
    ```
7. 将编译好的libudf安装到目标路径
    ```sh
    xmake install -o D:/path/to/fluent/solution/workdir libudf
    ```

8. 在FLUENT中导入libudf

### Linux

- 暂不直接支持。(不过linux直接改makefile也还挺方便，直接到安装的Ansys目录里面找到"{ANSYS INSTALL DIR}/v241/fluent/fluent24.1.0/src/udf"，把里面的东西拷贝出来用按需求改一改然后直接用make生成也凑合)。

## 推荐编辑环境

vscode + clangd插件 + XMake插件 + CodeLLDB插件(*用不太到，还是用VS调试UDF更方便*)

调试可以用命令```xmake project -k cmake```生成CMakeLists.txt或者```xmake project -k vsxmake```生成vs的sln项目，然后用VS附加到进程调试更方便。

## 基本示例

## TODO
