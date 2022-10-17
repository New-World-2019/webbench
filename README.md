# Web Bench 开源项目讲解

Web Bench 主要包括三个文件：webbench.c、socket.c、Makefile。
* webbench.c : 测压主文件
* socket.c : 将 socket 的连接封装成了函数
* Makefile : 程序的编译文件
[Web Bench 官网](http://home.tiscali.cz/~cz210552/webbench.html)

## Web Bench 流程图

<div align=center>
  <img src="./flowChart.png">
</div>

## 构建环境
```
Linux version 5.15.0-46-generic (buildd@lcy02-amd64-007) (gcc (Ubuntu 9.4.0-1ubuntu1~20.04.1) 9.4.0, GNU ld (GNU Binutils for Ubuntu) 2.34) #49~20.04.1-Ubuntu SMP Thu Aug 4 19:15:44 UTC 2022
```
## 构建方法
```
make
./webbench 参数信息
```
## 一、参数解析
1. 通过 main 函数接收参数 argc 和 argv;
2. C语言库函数 getopt_long 解析参数，其中 getopt_long 如下所是：

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
* argc ：参数个数
* argv : 参数数组
* optstring : 短选项字符串
* longopts : 长选项字符串
* longindex : longopts 的下标值，如果非空，表示当前参数符合 longopts 中的参数的下标
* 更详细内容可以参考：https://blog.csdn.net/qq_33850438/article/details/80172275

## 二、构建请求
1. 判断请求方法，将请求方法存储到 request 字符串数组中；
该请求是一个 http 的报文，例如：
```
GET / HTTP/1.0
User-Agent: WebBench 1.5
Host: www.baidu.com
```

## 三、打印测压配置信息

输出 方法、URL、HTTP协议版本、客户端数量、将要运行多长时间等


## 四、测压和输出

1. 通过 Socket 测试 host 是否通；
2. pipe 创建父子进程通信的管道；
3. fork 函数产生 clients 个客户端；
4. 子进程执行测压，父进程接收子进程测压结果，并进行统计，最后输出统计信息

## 五、实例演示
1. 输入参数
输入数据：测试 ./webbench -c 10 -t 15 http://www.baidu.com/
其中，各个参数表示如下：
-c 10 : 模拟 10 个客户端进行测压；
-t 15 : 测压时间为 15s；
http://www.baidu.com/ : 测压网站，需要写完整

2. 解析参数
参数 -c 解析为 clients = 10；
参数 -t 解析为 benchtime = 15;

3. 构建请求 && 输出测压信息
```
Req=GET / HTTP/1.0
User-Agent: WebBench 1.5
Host: www.baidu.com

Benchmarking: GET http://www.baidu.com/
10 clients, running 15 sec.
```
构建了一个 http 报文，通过 10 个客户端，每个 15s 进行测压

4. 输出测压信息
```
Speed=1388 pages/min, 8510421 bytes/sec.
Requests: 347 susceed, 0 failed.
```



