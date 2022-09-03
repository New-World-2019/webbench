/*
 * (C) Radim Kolar 1997-2004
 * This is free software, see GNU Public License version 2 for
 * details.
 *
 * Simple forking WWW Server benchmark:
 *
 * Usage:
 *   webbench --help
 *
 * Return codes:
 *    0 - sucess
 *    1 - benchmark failed (server is not on-line)
 *    2 - bad param
 *    3 - internal error, fork failed
 * 
 */ 
#include "socket.c"
#include <unistd.h>
#include <sys/param.h>
#include <rpc/types.h>
#include <getopt.h>
#include <strings.h>
#include <time.h>
#include <signal.h>

/* values */
volatile int timerexpired = 0; // 标记测试时间是否用完
int speed = 0;
int failed = 0; // 记录失败次数
int bytes = 0;  // 记录读取的字节数
/* globals */
/* 0 - http/0.9
   1 - http/1.0
   2 - http/1.1
*/
int http10 = 1; 
/* Allow: GET, HEAD, OPTIONS, TRACE */
#define METHOD_GET 0
#define METHOD_HEAD 1
#define METHOD_OPTIONS 2
#define METHOD_TRACE 3
#define PROGRAM_VERSION "1.5"
int method = METHOD_GET;
int clients = 1; // 进程个数，默认 1 个
int force = 0;   // 是否接受服务区的返回信息
int force_reload = 0;
int proxyport=80;
char *proxyhost=NULL;
int benchtime=30;
/* internal */
int mypipe[2];
char host[MAXHOSTNAMELEN];
#define REQUEST_SIZE 2048
char request[REQUEST_SIZE];

// 预先定义的长参数
static const struct option long_options[]=
{
 {"force",no_argument,&force,1},
 {"reload",no_argument,&force_reload,1},
 {"time",required_argument,NULL,'t'},
 {"help",no_argument,NULL,'?'},
 {"http09",no_argument,NULL,'9'},
 {"http10",no_argument,NULL,'1'},
 {"http11",no_argument,NULL,'2'},
 {"get",no_argument,&method,METHOD_GET},
 {"head",no_argument,&method,METHOD_HEAD},
 {"options",no_argument,&method,METHOD_OPTIONS},
 {"trace",no_argument,&method,METHOD_TRACE},
 {"version",no_argument,NULL,'V'},
 {"proxy",required_argument,NULL,'p'},
 {"clients",required_argument,NULL,'c'},
 {NULL,0,NULL,0}
};

// 函数声明
static void benchcore(const char* host,const int port, const char *request);
static int bench(void);
static void build_request(const char *url);

// 当 SIGALRM 信号触发时，调用此函数
static void alarm_handler(int signal)
{
   timerexpired = 1;
}	

// 参数说明
static void usage(void)
{
   fprintf(stderr,
	"webbench [option]... URL\n"
	"  -f|--force               Don't wait for reply from server.\n"   // 不等待服务器返回
	"  -r|--reload              Send reload request - Pragma: no-cache.\n"
	"  -t|--time <sec>          Run benchmark for <sec> seconds. Default 30.\n" // 运行测试多长时间
	"  -p|--proxy <server:port> Use proxy server for request.\n"  // 使用代理服务器
	"  -c|--clients <n>         Run <n> HTTP clients at once. Default one.\n" // 一次运行多少个客户端共同测试
	"  -9|--http09              Use HTTP/0.9 style requests.\n"  // 使用什么协议，支持长短参数
	"  -1|--http10              Use HTTP/1.0 protocol.\n"
	"  -2|--http11              Use HTTP/1.1 protocol.\n"
	"  --get                    Use GET request method.\n"  // 使用 GET 请求方法
	"  --head                   Use HEAD request method.\n" // 使用 HEAD 请求方法
	"  --options                Use OPTIONS request method.\n" // 使用 OPTIONS 请求方法
	"  --trace                  Use TRACE request method.\n"  // 使用 TRACE 请求方法
	"  -?|-h|--help             This information.\n"        // 显示帮助信息
	"  -V|--version             Display program version.\n" // 显示版本信息
	);
};

//主函数
int main(int argc, char *argv[])
{
   // 1. 参数解析
   int opt = 0;  // 接收 getopt_long 返回结果
   int options_index = 0;
   char *tmp = NULL;

   // 如果一个参数，参数错误，输出参数规则
   if(argc==1) {
	   usage(); // 参数说明
      return 2;
   } 

   // 使用 getopt_long 函数解析参数，支持长短参数
   while((opt = getopt_long(argc,argv, "912Vfrt:p:c:?h", long_options, &options_index)) != EOF) {
      switch(opt) {
         case  0 : break;
         case 'f': force=1;break;
         case 'r': force_reload=1;break; 
         case '9': http10=0;break;
         case '1': http10=1;break;
         case '2': http10=2;break;
         case 'V': printf(PROGRAM_VERSION"\n");exit(0);
         case 't': benchtime=atoi(optarg);break; // 转化为测压时间	     
         case 'p': // 设置了代理
	         /* 解析代理服务器 server:port */
	         tmp = strrchr(optarg,':'); // tmp 为 port
            proxyhost = optarg;
            if(tmp == NULL) {
               break;
            }
            if(tmp==optarg) { // 缺少 hostname
               fprintf(stderr,"Error in option --proxy %s: Missing hostname.\n",optarg);
               return 2;
            }
            if(tmp==optarg+strlen(optarg)-1) { //缺少 port
               fprintf(stderr,"Error in option --proxy %s Port number is missing.\n",optarg);
               return 2;
            }
            *tmp='\0';
            proxyport=atoi(tmp+1);break; //将 port 转化为数字，此时，proxyhost 和 proxyport 都解析出来了
         case ':':
         case 'h':
         case '?': usage();return 2;break; // ：/h/? 都输出参数说明
         case 'c': clients=atoi(optarg);break; // 转化为客户端个数
      }
   }
 
   if(optind == argc) { // 没有 URL
      fprintf(stderr,"webbench: Missing URL!\n");
		usage();
		return 2;
   }

   if(clients == 0) clients = 1; // 默认 clients 为 1
   if(benchtime == 0) benchtime = 60; // 默认 benchtime 为 60s
   /* Copyright */
   fprintf(stderr,"Webbench - Simple Web Benchmark "PROGRAM_VERSION"\n"
	   "Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.\n");

   // 2. 构建请求
   build_request(argv[optind]);

   // 3. 输出测压信息
   printf("\nBenchmarking: ");
   switch(method) {
	   case METHOD_GET:
	   default:
		   printf("GET");break;
	   case METHOD_OPTIONS:
		   printf("OPTIONS");break;
	   case METHOD_HEAD:
		   printf("HEAD");break;
	   case METHOD_TRACE:
		   printf("TRACE");break;
   }
   printf(" %s",argv[optind]); // 输出 URL
   switch(http10) // 输出使用的 HTTP 协议版本
   {
	   case 0: printf(" (using HTTP/0.9)");break;
	   case 2: printf(" (using HTTP/1.1)");break;
   }
   printf("\n");
   if(clients==1) printf("1 client");
   else
      printf("%d clients",clients);

   printf(", running %d sec", benchtime);
   if(force) printf(", early socket close");
   if(proxyhost!=NULL) printf(", via proxy server %s:%d",proxyhost,proxyport);
   if(force_reload) printf(", forcing reload");
   printf(".\n");

   return bench();
}

// 构建请求
void build_request(const char *url)
{
   char tmp[10];
   int i;

   // 初始化
   bzero(host,MAXHOSTNAMELEN);
   bzero(request,REQUEST_SIZE);

   // 1. 检查通信协议，添加请求方法到 request
   if(force_reload && proxyhost != NULL && http10 < 1) http10 = 1;
   if(method == METHOD_HEAD && http10 < 1) http10 = 1;
   if(method == METHOD_OPTIONS && http10 < 2) http10 = 2;
   if(method == METHOD_TRACE && http10 < 2) http10 = 2;

   switch(method) {
	   default:
	   case METHOD_GET: strcpy(request,"GET");break;
	   case METHOD_HEAD: strcpy(request,"HEAD");break;
	   case METHOD_OPTIONS: strcpy(request,"OPTIONS");break;
	   case METHOD_TRACE: strcpy(request,"TRACE");break;
   }
  
   // 2. 添加空格
   strcat(request," ");

   // 3. 检查 URL
   if(NULL==strstr(url,"://")) {
	   fprintf(stderr, "\n%s: is not a valid URL.\n",url);
	   exit(2);
   }
   // 检查 url 长度
   if(strlen(url) > 1500) {
      fprintf(stderr,"URL is too long.\n");
	   exit(2);
   }
   
   // 如果代理为空，且 url 不是以 http:// 开头，则输出错误
   if(proxyhost == NULL)
	   if (0 != strncasecmp("http://", url, 7)) {
         fprintf(stderr,"\nOnly HTTP protocol is directly supported, set --proxy for others.\n");
         exit(2);
      }

   /* protocol/host delimiter */
   i = strstr(url, "://") - url + 3;

   if(strchr(url+i, '/') == NULL) {
      fprintf(stderr,"\nInvalid URL syntax - hostname don't ends with '/'.\n");
      exit(2);
   }
   if(proxyhost == NULL) {
      /* get port from hostname */
      if(index(url+i,':')!=NULL && index(url+i,':')<index(url+i,'/')) {
	      strncpy(host,url+i,strchr(url+i,':')-url-i);
	      bzero(tmp,10);
	      strncpy(tmp,index(url+i,':')+1,strchr(url+i,'/')-index(url+i,':')-1);
	      /* printf("tmp=%s\n",tmp); */
	      proxyport=atoi(tmp);
	      if(proxyport==0) proxyport=80;
      } else {
         strncpy(host,url+i,strcspn(url+i,"/"));
      }
      // printf("Host=%s\n",host);
      strcat(request+strlen(request),url+i+strcspn(url+i,"/"));
   } else {
      // printf("ProxyHost=%s\nProxyPort=%d\n",proxyhost,proxyport);
      strcat(request,url);
   }
  
   // 处理通信协议
   if(http10 == 1)
	   strcat(request," HTTP/1.0"); // 将 " HTTP/1.0" 追加到 request 后
   else if (http10 == 2)
	   strcat(request," HTTP/1.1");
   strcat(request,"\r\n");
   if(http10 > 0)
	   strcat(request,"User-Agent: WebBench "PROGRAM_VERSION"\r\n");
   if(proxyhost == NULL && http10 > 0) {
	   strcat(request,"Host: ");
	   strcat(request,host);
	   strcat(request,"\r\n");
   }

   if(force_reload && proxyhost!=NULL) {
	   strcat(request,"Pragma: no-cache\r\n");
   }
   if(http10 > 1)
	   strcat(request,"Connection: close\r\n");
      /* add empty line at end */
      if(http10>0) strcat(request,"\r\n"); 
  
   // 打印 request
   printf("Req=%s\n",request);
}

/* vraci system rc error kod */
static int bench(void)
{
  printf("function bench\n");
  int i, j, k;	
  pid_t pid = 0;
  FILE *f;

   // 检查目标服务器是否可用
   printf("host = %s\n", host);// www.baidu.com
   i = Socket(proxyhost == NULL ? host:proxyhost,proxyport);
   if(i < 0) { 
      fprintf(stderr,"\nConnect to server failed. Aborting benchmark.\n");
      return 1;
   }
   close(i);

   // 创建管道，用于父子进程之间通信
   if(pipe(mypipe)) {
	   perror("pipe failed.");
	   return 3;
   }

  /* not needed, since we have alarm() in childrens */
  /* wait 4 next system clock tick */
  /*
  cas=time(NULL);
  while(time(NULL)==cas)
        sched_yield();
  */

   // 使用 fork 产生子进程
   for(i = 0; i < clients; i++) {
	   pid = fork();
	   if(pid <= (pid_t) 0) {
		   /* child process or error*/
	      sleep(1); /* make childs faster */
		   break;
	   }
   }

   if( pid < (pid_t) 0) {
      fprintf(stderr,"problems forking worker no. %d\n",i);
	   perror("fork failed.");
	   return 3;
   }

   // 子进程
   if(pid == (pid_t) 0) {
      // 子进程执行任务
      if(proxyhost == NULL)
         benchcore(host,proxyport,request);
      else
         benchcore(proxyhost,proxyport,request);

      /* write results to pipe */
	   f = fdopen(mypipe[1],"w");
	   if(f == NULL) {
		   perror("open pipe for writing failed.");
		   return 3;
	   }

      // 将自身数据写入到管道中，供父进程读取
	   fprintf(f,"%d %d %d\n",speed,failed,bytes);
	   fclose(f); // 关闭管道
	   return 0;
   } else {   // 父进程
      // 以读的方式打开管道
	   f = fdopen(mypipe[0],"r");
      if(f == NULL) {
		   perror("open pipe for reading failed.");
		   return 3;
	   }

      // 初始化
	   setvbuf(f,NULL,_IONBF,0);
	   speed = 0;
      failed = 0;
      bytes = 0;

      //不断读取子进程中的数据
	   while(1) {
		   pid = fscanf(f,"%d %d %d",&i,&j,&k);
		   if(pid < 2) {
            fprintf(stderr,"Some of our childrens died.\n");
            break;
         }
		   speed += i;
		   failed += j;
		   bytes += k;
		  // 读取完所有子进程则跳出循环
		  if(--clients == 0) break;
      }
	   fclose(f);

      printf("\nSpeed=%d pages/min, %d bytes/sec.\nRequests: %d susceed, %d failed.\n",
		  (int)((speed+failed)/(benchtime/60.0f)),
		  (int)(bytes/(float)benchtime),
		  speed,
		  failed);
  }
  return i;
}

/**
 * @brief 与服务器进行连接
 * 
 * @param host 
 * @param port 
 * @param req 
 */
void benchcore(const char *host,const int port,const char *req)
{
   int rlen;
   char buf[1500];  // 用于读取返回值
   int s, i;
   struct sigaction sa;

   /* setup alarm signal handler */
   sa.sa_handler = alarm_handler;
   sa.sa_flags = 0;
   // 当 SIGALRM 信号触发时，调用 alarm_handler 函数
   if(sigaction(SIGALRM, &sa, NULL))
      exit(3);
   
   // 设置时钟，benchtime 时间后触发 SIGALRM 信号
   alarm(benchtime);

   // 计算 req 长度
   rlen = strlen(req);

   // 使用了 goto 语句，执行 goto nexttry 语句时，回到此处
   nexttry:while(1) {
      // 检测通信时间是否到了
      if(timerexpired) {
         if(failed > 0) {
            /* fprintf(stderr,"Correcting failed by signal\n"); */
            failed--;
         }
         return;
      }

      // 创建一个 socket，Socket 为 socket.c 文件中封装的函数
      s = Socket(host, port);                          
      if(s < 0) { 
         failed++;
         continue;
      }

      // 发送数据
      if(rlen != write(s,req,rlen)) {
         failed++;
         close(s);
         continue;
      }

      if(http10 == 0) {
         if(shutdown(s,1)) { // 关闭 socket 写
            failed++;
            close(s); // 关闭失败后，使用 close 关闭
            continue;
         }
      } 
	   
      // force 等于 0 需要等待服务器返回结果
      if(force == 0) {
         /* read all available data from socket */
	      while(1) {
            if(timerexpired) break; 
	         i = read(s,buf,1500);
            /* fprintf(stderr,"%d\n",i); */
	         if(i < 0) { 
               failed++;
               close(s);
               goto nexttry;
            } else {
               if(i == 0) break;
		         else
			         bytes += i;
            }
		         
	      }
      }

      if(close(s)) {
         failed++;
         continue;
      }
      speed++;
   }
}
