# FLUENT UDF

## 介绍

- 简介：使用xmake构建FLUENT UDF

- 拟解决的问题：FLUENT内置的编译脚本有点坑，例如:
    1. DEFINE_XX宏那一行不能有多余字符，导致我习惯的左大括号放在上一行不换行会导致生成udf_names.c出错；
    2. 在部分版本的FLUENT上内置编译脚本匹配DEFINE_XX宏时有时会漏掉部分内容；
    3. 当系统语言是中文时，用FLUENT内的编译udf，如果代码有错误，编译报错的提示在FLUENT的console中会乱码，而且信息多且乱；
    4. 编译器开启的语言标准采用默认，现在想开c++20标准还得去改他的编译脚本；
    5. 调用第三方库麻烦。

## 说明

本脚本通过```AWP_ROOT<ver>```环境变量寻找FLUENT的安装位置，需要提前设置环境变量。

（例如，环境变量```AWP_ROOT241```中指定ANSYS2024R1的安装目录```"C:\Program Files\ANSYS2024R1\v241"```）

一般Windows上正常安装完成后ANSYS的安装程序已经自动生成了一系列的环境变量，不需要用户手动设置了。

## 安装步骤

### Windows

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
    xmake install -o D:/path/to/fluent/solution/workdir libudf # xmake<=2.9.5
    xmake install -o D:/path/to/fluent/solution/workdir        # xmake>=2.9.6
    ```

8. 在FLUENT中导入libudf

### Linux

- 暂不直接支持。(没有测试环境)。

## 推荐环境

vscode + clangd + xmake

Windows下调试可以用命令```xmake project -k cmake```生成CMakeLists.txt或者```xmake project -k vsxmake```生成vs的sln项目，然后用VS附加到进程调试更方便。

## 基本示例

## TODO
