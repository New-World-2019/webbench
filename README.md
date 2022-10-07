# webbench 开源项目讲解

webbench 主要包括三个文件：webbench.c、socket.c、Makefile。
* webbench.c : 测压主文件
* socket.c : 将 socket 的连接封装成了函数
* Makefile : 程序的编译文件

## webbench 流程图

![webbench 流程图](./flowChart.png#pic_center)
<div align=center>
  <img src="./flowChart.png">
</div>

## 一、参数解析
### 1. 通过 main 函数接收参数 argc 和 argv;
### 2. C语言库函数 getopt_long 解析参数，其中 getopt_long 如下所是：

头文件：
```
#include <getopt.h>
```
函数原型:
```
int getopt_long(int argc, char * const argv[], const char *optstring,
                const struct option *longopts, int *longindex);
```
各个参数含义如下：
argc ：参数个数
argv : 参数数组
optstring : 短选项字符串
longopts : 长选项字符串
longindex : longopts 的下标值，如果非空，表示当前参数符合 longopts 中的参数的下标
更详细内容可以参考：https://blog.csdn.net/qq_33850438/article/details/80172275

## 二、构建请求
### 1. 判断请求方法，将请求方法存储到 request 字符串数组中；
该请求是一个 http 的报文，例如：
```
GET / HTTP/1.0
User-Agent: WebBench 1.5
Host: www.baidu.com
```

## 三、打印测压配置信息

输出 方法、URL、HTTP协议版本、客户端数量、将要运行多长时间等


## 四、测压和输出

### 1. 通过 Socket 测试 host 是否通；
### 2. pipe 创建父子进程通信的管道；
### 3. fork 函数产生 clients 个客户端；
### 4. 子进程执行测压，父进程接收子进程测压结果，并进行统计，最后输出统计信息




