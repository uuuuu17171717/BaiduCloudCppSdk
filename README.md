Windows版优化官方SDK，带上传进度

# Baidu Object Storage(BOS) SDK implemented by C++

BOS C++ SDK是Linux的C++开发工具包，提供完备的BOS API

[TOC]

## Linux用户
###依赖
依赖开源的第三方库，包括curl、openssl、uuid、jsoncpp，其中jsoncpp
可在thirdlib文件夹下编译并安装:
```
cd thirdlib
make clean
make
make install
```

上述命令即可再thirdlib中生成jsoncpp库。其他库请自行安装，建议使用yum install。

### 环境构建
#### 基于autotools
1. 执行sh autogen.sh生成configure
2. 执行configure，添加必要的选项，比如--prefix或者CXXFLAGS等
3. 执行make & make install，将在对应路径生成include和lib

### 编译引用
#### Makefile
1. 如果没有安装到gcc默认路径下，则在CXXFLAGS里使用-L指定lib路径，使用-I指定include路径
2. 在LDFLAGS里指定-lbossdk

如果直接写编译命令的，可也以直接指定-L -I -l




## Windows用户
### 环境准备
* 安装Visual studio 2012或更高版本。
* 安装CMake3.1或以上。
* thirdlib已经包含所需要三方库（curl、jsoncpp），如需要其它版本，请到官方网站下载。

### 快速体验
1.解压thirdlib目录下的windows_dependency.zip到thirdlib下。
2.双击example\example.sln


### 编译使用
1.解压thirdlib目录下的windows_dependency.zip到thirdlib下。
2.创建build目录
``` 
mkdir build
cd build
```

3.生成目标sln
``` cmake -G "<type>" .. ```
这里要根据所安装的Visual studio 版本和编译平台，而选择不同的```<type>```。
> 生成Visual Studio  2015的sln： ``` cmake -G "Visual Studio 14 2015" .. ```
> 生成Visual Studio  2013编译目标是64位的sln： ``` cmake -G "Visual Studio 12 2013 Win64" .. ```

参数详见CMake帮助文档。

4.编译
直接使用cmake 编译 ``` cmake --build .```
或手动打开bossdk.sln

5.结果生成
编译结果将生成在代码根目录的output文件夹下。结构如下：
```
output
├── include
│   └── bcesdk
│       ├── auth
│       ├── bos
│       │   ├── model
│       │   ├── request
│       │   └── response
│       ├── common
│       ├── http
│       ├── model
│       └── util
└── lib
    ├── x64
    │   ├── Debug
    │   └── Release
    └── x86
        ├── Debug
        └── Release
```

6.在使用SDK
* 编译：把output/include添加include 目录下
* 链接：主动链接bossdk.lib，jsoncpp.lib，LIBCURL.LIB
* 执行：把LIBCURL.DLL放到目标exe同一个目录下


### 低于Visual studio 2012 的编译问题参考
* 低于Visual studio 2005 将存在大量问题，强烈不建议使用。
* 必须使用windows SDK 7.1 或以上版本。
* 需要解决stdint.h不存在问题和替换少量的CRT函数如：strtoull，strtoll等。


### 运行环境支持
Windows XP SP3及以上版本。




## 开发中使用

### 头文件
```cpp
#include <bcesdk/bos/client.h>
```

### Windows下初始化
由于SDK代码中使用Windows sockets api，使用前必须要先执行WSAStartup。
详见： https://msdn.microsoft.com/en-us/library/windows/desktop/ms742213(v=vs.85).aspx

### 初始化
```cpp
    // 默认选项
    Client client(ak, sk);
        ...
    // 使用自定义选项
    baidu::bos::cppsdk::ClientOptions option;
    option.endpoint = "bj.bcebos.com";
    Client client(ak, sk, option);
        ...
    // 使用sts
    Client client(Credential(ak, sk, sts_token), option);
```
ClientOptions说明：

|字段|默认值|描述|
| - | - | - |
|endpoint|必填|需要填写标准endpoint，这个在百度云BOS API文档上可以查到
|timeout|120s|是秒为单位的超时，这个要根据请求类型慎重选取，比如下载一个大文件可能用到比较大的超时
|retry|2|额外的重试次数
|user_agent|cppsdk相关|可以指定请求发送的时候的UA
|max_parallel|10|并发请求接口的默认并发数（也可以直接在调用接口时参数指定）
|multi_part_size|10MB|功能性函数里封装三步上传或者并发下载的块大小
|sign_expire_seconds|1200s|指定签名超时的时间

### 线程安全
除了Client对象本身的构造过程，以下将提到的接口方法都是线程安全的，这意味着可以在多线程环境里访问同一个Client对象的各个接口。
这里要注意，Client对象的参数（request&response请求对象）不是线程安全的。

### REST API接口
#### 接口形式
调用方式都是
```cpp
method_name(request, &response);
```
通过构建request对象来传入请求的参数（bucket名，对象名，需要上传的文件流，要保存的文件流）等等。
然后通过response获取请求的状态（成功与否），以及请求结果。

#### 错误处理
通过response对象的is_fail()或者is_ok()方法来直接判断请求是否成功。
通过response对象的status_code()来获取失败情况下的http状态码（或者本地状态码）。
通过response对象的error()来获取异常对象，同时通过其code()和message()方法来获取百度云的错误代码和具体错误信息。
此外一定要打印出request_id和debug_id，这是排查问题时候的重要依据。
示例如下：
```cpp
	printf("status:%d\n", response.status_code());
    if (response.is_ok()) {
        printf("request-id:%s\n", response.request_id().c_str());
        printf("debug-id:%s\n", response.debug_id().c_str());
    }
    if (response.is_fail()) {
        printf("error-message:%s\n", response.error().message().c_str());
    }
```

#### 便利性接口
便利接口将常用的请求参数和访问接口进行封装，使得用户不用构造请求对象，同时根据返回值判断成功与否。
如下：
```cpp
    // 将文本信息或者内存中的数据上传到bos
    int put_object(const std::string &bucket_name, const std::string &object_name, const std::string &data) const;
    // 将bos上的数据，比如图片之类的下载到内存中
    int get_object(const std::string &bucket_name, const std::string &object_name, std::string *data) const;
    // 删除
    int delete_object(const std::string &bucket_name, const std::string &object_name) const;
    // 生成可供公开发布的，带签名时效的下载链接，时效为-1时表示永久有效
    std::string generate_url(const std::string &bucket, const std::string &object, int expire_seconds = -1);
```

### 并发接口
并发接口基于curl的多路IO实现，仍然使用单线程（调用者线程），不会额外创建线程，但是通过非阻塞IO提高请求数的吞吐。
基本接口如下：
```cpp
    struct BceRequestContext {
        BceRequest *request;
        BceResponse *response;
        bool is_own;
    };
    int send_request(int n, BceRequestContext ctx[], int max_parallel = 0);
```

Context的request和response成员用来放置请求对象的指针；is_own为true时将在Context析构时主动释放request和response指针。
send_request方法中
    n为需要并发执行的请求数，max_parallel为最大并发数（默认将按options设置的来）；
    当请求数超过最大并发数时，剩下的请求将排队等待，直到有请求完成再加入并发；
    通过方法的返回值判断是否所有的请求都执行成功，同时可以遍历ctx数组挨个判断每个response的结果。

#### 便利性接口
```cpp
    // 并发下载一个文件（或者指定的区间）到文件流中，可同时附带获取元信息
    int parallel_download(const std::string &bucket, const std::string &object, FileOutputStream &file,
            int64_t start = 0, int64_t size = -1, ObjectMetaData *meta = NULL);
    // 并发上传一个文件到BOS，同时可附带上传元信息
    int parallel_upload(const std::string &bucket, const std::string &object, FileInputStream &file,
            ObjectMetaData *meta = NULL);
    // 并发在BOS服务端复制一个文件，同时设置存储类型（为空时表示不改变存储类型）
    int parallel_copy(const std::string &src_bucket, const std::string &src_object,
            const std::string &dst_bucket, const std::string &dst_object,
            const std::string &storage_class);
```

### 流对象
流对象目前主要用于访问文件，用户只需构造好并传给相应的请求对象，请求对象会有is_own标识来设置由谁负责释放流对象。
示例如下：
```cpp
    // 使用文件名，下载到文件特定位置
    FileOutputStream *file = new FileOutputStream("myfile", download_to_offset);
        ...
    // 多个流共享fd
    int fd = open("myfile", O_RDWR);
    FileOutputStream *file = new FileOutputStream(fd);
        ...
    // 传入请求对象，由请求对象负责释放
    GetObjectResponse response(file, true);
        ...
    // 上传文件的特定位置和大小
    FileInputStream *file = new FileInputStream("myfile", start_offset, length);
```

更详细的接口信息可以阅读相应的头文件了解

## 厂内用户
### BCLOUD
指定CONFIGS('baidu/bos/cppsdk@{tagname}@git_tag')，自行选定想依赖的tag版本，比如cppsdk_1-0-3-3_PD_BL

### BNS
endpoint可以指定集群的BNS，格式为：bns://xxxx.all

------
Copyrights 2016 Baidu, Inc. all reserved.

