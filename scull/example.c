#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/fcntl.h>

int main()
{
 int fd;
 char buf[]="this is a example for character devices driver by Liao712148!";//寫入scullc0裝置的內容

 char buf_read[4096];//scullc0裝置的內容讀入到該buf中

 
 if((fd=open("/dev/scullc0",O_RDWR))==-1)//開啟scullc0裝置

  printf("open scullc0 WRONG！\n");
 else
  printf("open scullc0 SUCCESS!\n");
  
 printf("buf is %s\n",buf);

 write(fd,buf,sizeof(buf));//把buf中的內容寫入scullc0裝置

 
 lseek(fd,0,SEEK_SET);//把檔案指標重新定位到檔案開始的位置

 
 read(fd,buf_read,sizeof(buf));//把scullc0裝置中的內容讀入到buf_read中

 
 printf("buf_read is %s\n",buf_read);
 
 return 0;
}
