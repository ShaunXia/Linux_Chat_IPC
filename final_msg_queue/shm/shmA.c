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
    //创建共享内存段  
    int shmid = shmget(key,12,IPC_CREAT|IPC_EXCL|0600);  
    if(shmid<0)perror("error"),exit(-1);  
    //挂接共享内存段  
    void *p = shmat(shmid,NULL,0);  
    if(p==(void*)-1)perror("error"),exit(-1);     
    //获取共享内存段的详细信息  
    struct shmid_ds ds;  
    bzero(&ds,sizeof(ds));  
    if(shmctl(shmid,IPC_STAT,&ds)<0)perror("error");  
    printf("uid=%d\n",ds.shm_perm.uid);  
    printf("mode=%o\n",ds.shm_perm.mode);  
    printf("size=%d\n",ds.shm_segsz);  
    printf("cpid=%d\n",ds.shm_cpid);  
    printf("lpid=%d\n",ds.shm_lpid);  
      
    //使用共享内存段进行通信  
    int *f = p;  
    *f = 1; //在此模仿信号量。 为1可写数据，为0可读数据。  
    int i;  
    for(i=100;i<120;i++)  
    {  
        while(*f==0); //等待别的进程取数据  
        *(f+1) = i;  
        *f = 0;  
        sleep(1);  
    }  
    //脱接共享内存段  
    if(shmdt(p)<0)perror("error"),exit(-1);  
    //删除共享内存段  
    if(shmctl(shmid,IPC_RMID,NULL)<0)perror("error"),exit(-1);  
}  
