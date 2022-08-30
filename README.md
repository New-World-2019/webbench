# webbench 开源项目讲解



## 参数解析
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
argc ：参数个数
argv : 参数数组
optstring : 短选项字符串
longopts : 长选项字符串
longindex : longopts 的下标值，如果非空，表示当前参数符合 longopts 中的参数的下标
更详细内容可以参考：https://blog.csdn.net/qq_33850438/article/details/80172275

## 构建请求
1. 判断请求方法，将请求方法存储到 request 字符串数组中；
2. 



