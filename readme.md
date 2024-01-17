# FLUENT UDF

简介：使用xmake构建FLUENT UDF项目

## 基本使用步骤

### Windows 10+

1. 安装C/C++环境 (已安装跳过) (*推荐Visual Studio Community 2022，安装时工作负载勾选"使用C++的桌面开发"*)

2. 安装ANSYS Fluent (已安装跳过) (*测试过2020R1~2024R1都能用，更低的理论上也行但是没试过*)

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

```暂不支持，因为现在我用不到。```

## 基本示例

## TODO
