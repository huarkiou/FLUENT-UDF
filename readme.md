# FLUENT UDF

- 简介：使用xmake构建FLUENT UDF

- 拟解决的问题：FLUENT内置的编译脚本有点坑，例如: 1. DEFINE_XX宏那一行不能有多余字符，导致我习惯的左大括号放在上一行不换行会导致生成udf_names.c出错；2. 想开c++20标准还得去改他的编译脚本；3. 调用第三方库麻烦；4. 最开始配置UDF的编译环境有很多小坑，虽然很好解决，但是令人不爽。

## 基本使用步骤

### Windows 10+

1. 安装C/C++环境 (**已安装则跳过**) (*推荐Visual Studio Community 2022，安装时工作负载勾选"使用C++的桌面开发"*)

2. 安装ANSYS Fluent (**已安装则跳过**) (*测试过2020R1~2024R1都能用，更低的理论上也行但是没试过*)

3. 安装[xmake](https://github.com/xmake-io/xmake/releases/)，确保已将其添加至环境变量

4. 编译UDF生成libudf
    ```sh
    git clone https://github.com/huarkiou/FLUENT-UDF.git # 下载本仓库内容
    cd FLUENT-UDF # 进入新下载的目录
    xmake build libudf # 编译生成UDF动态链接库
    ```
5. 将编译好的libudf安装到目标路径
    ```sh
    xmake install -o D:/path/to/fluent/solution/workdir libudf
    ```

6. 在FLUENT中导入libudf

### Linux

- 暂不支持，因为现在我用不到，而且在我的ARCHLINUX上装ANSYS太麻烦了。不过linux直接改makefile也还挺方便，直接到安装的Ansys目录里面找到"{ANSYS INSTALL DIR}/v241/fluent/fluent24.1.0/src/udf"，把里面的东西拷贝出来用按需求改一改然后直接用make生成也凑活。

## 基本示例

## TODO
