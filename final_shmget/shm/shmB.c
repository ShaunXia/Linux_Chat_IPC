#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>  
#include <sys/shm.h>  
#include <sys/ipc.h>  
  
int main()  
{  
    //生成key  
    key_t key = ftok(".",1002);  
    //获取共享内存段  
    int shmid = shmget(key,0,0);  
    if(shmid<0)perror("error"),exit(-1);  
    //挂接共享内存段  
    void *p = shmat(shmid,NULL,0);  
    if(p==(void*)-1)perror("error"),exit(-1);  
    //获取共享内存段信息  
    struct shmid_ds ds;  
    bzero(&ds,sizeof(ds));  
    if(shmctl(shmid,IPC_STAT,&ds)<0)perror("error"),exit(-1);  
    printf("uid=%d\n",ds.shm_perm.uid);  
    printf("mode=%o\n",ds.shm_perm.mode);  
    printf("size=%d\n",ds.shm_segsz);  
    printf("cpid=%d\n",ds.shm_cpid);  
    printf("lpid=%d\n",ds.shm_lpid);  
    //使用共享内存进行进程通信  
    int *f = p;  
    while(1)  
    {  
        while(*f);//等待另一个进程放入数据  
        printf("%d ",*(f+1));  
        fflush(stdout);  
        *f = 1;  
          
    }  
    //脱接共享内存段  
    if(shmdt(p)<0)perror("error"),exit;  
}  
