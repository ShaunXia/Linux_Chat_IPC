#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <sys/shm.h>  
#include <sys/ipc.h>  
#include <sys/msg.h>
#include "comm_struct.h"

#define SERVER_MESSAGE_FIFO "./fifo/server/message_fifo"
#define SERVER_STAT "fifo/server/stat_fifo"
#define SERVER_STAT_BACK "fifo/server/stat_back"
#define SERVER_CLIENT_INFO "./fifo/server/clientinfo" 
#define MESSAGE_CLIENT_FIFO "./fifo/client/msg_"
#define NOTIFICATION_FROM_SERVER "./fifo/client/noti_"
char user[100][20];
char pwd[100][20];
int user_status[100];
int user_count=0;
int user_pid[100];
int online_user_num=0;
int ser_msgid,client_msgid;
void handler(int sig)
{
    unlink(SERVER_MESSAGE_FIFO);
    unlink(SERVER_STAT);
    unlink(SERVER_STAT_BACK);
    unlink(SERVER_CLIENT_INFO);
    //if(shmdt(p)<0)perror("error"),exit(-1);  
    exit(1);
}

void ServerDeamon()
{   
    int pid;
    int i;
    pid=fork();
    if(pid!=0)
        exit(0);        //是父进程，结束父进程
    else if(pid< 0)
        exit(1);        //fork失败，退出
    //是第一子进程，后台继续执行
    setsid();           //第一子进程成为新的会话组长和进程组长
    //并与控制终端分离
    //for(i=0;i< NOFILE;++i)  //关闭打开的文件描述符
    //   close(i);
    //chdir("/tmp");      //改变工作目录到/tmp
}
void DeamonCTL() //服务管理 线程 start stop status
{
    while(1){
        int fifo_fd = open(SERVER_STAT_BACK,O_RDONLY|O_NONBLOCK);
        if (fifo_fd!=-1)
            exit(0);
    }

}
void Client_Check() //客户端状态线程
{
    online_user_num=0;
    Client_info info;
    int res;
    int fifo_fd = open(SERVER_CLIENT_INFO,O_RDONLY);
    if(fifo_fd == -1)
    {
        printf("Could not open %s for read only access\n",SERVER_CLIENT_INFO);
        exit(EXIT_FAILURE);
    }
    int user_exist,i;
    char info_back;
    Notification noti;
    while(1)
    {
        user_exist=0;
        info_back='0';

        res = read(fifo_fd,&info,sizeof(Client_info));
        if(info.status==0)
        {
            info_back='1';
            user_exist=1;
            for(int i=0;i<user_count;++i)
            {
                if(!strcmp(user[i],info.userName))
                {
                    user_status[i]=0;
                }
            }
            //user_count--;
            online_user_num--;
        }
        if(info.status==1)
        {
            for(i=0;i<user_count;++i)
            {
                if(!strcmp(user[i],info.userName))
                {
                    user_exist=1;
                    if(!strcmp(pwd[i],info.passWord))
                    {
                        info_back='1';
                        user_status[i]=1;
                        user_pid[i]=info.client_pid;
                        online_user_num++;
                        break;
                    }
                    else
                    {
                        info_back='0'; 
                        break;
                    }
                }
            }
            if(user_exist==0)
            {
                sprintf(user[user_count],"%s",info.userName);
                sprintf(pwd[user_count],"%s",info.passWord);
                user_status[user_count] = info.status;
                info_back='2';
                user_pid[i]=info.client_pid;
                user_count++;

                online_user_num++;
            }
        }
        if(res>0)
        {

            if(info_back!='0')
            {
                FILE *fp=fopen("x.txt","wb");

                for(i=0;i<user_count;++i) //INFO ALL USER 
                {
                    char tp_fifo[200];
                    sprintf(tp_fifo,"%s%s",NOTIFICATION_FROM_SERVER,user[i]);
                    int tp_fifo_fd =  open(tp_fifo,O_WRONLY|O_NONBLOCK);
                    fprintf(fp,"%d\t%d\t%s\t%s\t%d\t%s\n",tp_fifo_fd,i,user[i],pwd[i],user_status[i],tp_fifo);
                    sprintf(noti.userName,"%s",info.userName);
                    noti.log_stat = info.status;

                    noti.user_count=user_count;
                    noti.noti_type=1;
                    write(tp_fifo_fd,&noti,sizeof(Notification));
                    close(tp_fifo_fd);

                }



                fclose(fp);

            }

            if(info.status==1) //Write Login Check FIFO
            {

                int fd1 = open(info.log_check_fifo,O_WRONLY|O_NONBLOCK);
                write(fd1,&info_back,sizeof(char)+1);
                close(fd1);
            }

        }

    }
}

int main(int argc,char *argv[])
{
    signal(SIGINT,  handler);// 终端终端  
    signal(SIGHUP,  handler);// 连接挂断  
    signal(SIGQUIT, handler);// 终端退出  
    signal(SIGPIPE, handler);// 向无读进程的管道写数据  
    signal(SIGTTOU, handler);// 后台程序尝试写操作  
    signal(SIGTTIN, handler);// 后台程序尝试读操作  
    signal(SIGTERM, handler);// 终止 

    int res;
    if(argc!=2)
    {
        printf("Usage: MessageServer start / stop / status\n");
        exit(0);
    }
    //=======Start Serverdeamon=========
    //
    if(!strcmp("start",argv[1]))
    {


        int _running=1;
        ServerDeamon(); //SET Deamon 
        //Create FIFO

        Comm_struct msg_from_client_queue,msg_to_client_queue;


        ser_msgid = msgget((key_t)1111,0666|IPC_CREAT);
        if(ser_msgid == -1)
        {
            exit(EXIT_FAILURE);
        }




        if (access(SERVER_MESSAGE_FIFO,F_OK) == -1)
        {
            res = mkfifo(SERVER_MESSAGE_FIFO,0777);
            if(res != 0 )
            {
                printf("FIFO %s was not created\n",SERVER_MESSAGE_FIFO);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            printf("Server Is RUNNING\n");
            exit(0);
        }


        if (access(SERVER_CLIENT_INFO,F_OK) == -1)
        {
            res = mkfifo(SERVER_CLIENT_INFO,0777);
            if(res != 0 )
            {
                printf("FIFO %s was not created\n",SERVER_CLIENT_INFO);
                exit(EXIT_FAILURE);
            }
        }


        pthread_t server_control;
        pthread_t client_check;

        Comm_struct msg_from_client;

        int control_ret = pthread_create(&server_control,NULL,(void *)DeamonCTL,NULL);                     
        int client_ret = pthread_create(&client_check,NULL,(void *)Client_Check,NULL);

        printf("Start! pid is %d\n",getpid());

        //Open SERVER fifo for reading
        int res,fifo_ser;
        fifo_ser=open(SERVER_MESSAGE_FIFO,O_RDONLY);
        if(fifo_ser==-1)
        {
            printf("OPEN ERROR\n");
            exit(EXIT_FAILURE);
        }

        //服务 主要 逻辑进程
        while(_running)
        {
            while(msgrcv(ser_msgid, &msg_from_client_queue, sizeof(Comm_struct), 0, 0)>0)
            { 

                int apid;

                for(int j=0;j<user_count;++j)
                {
                    if(!strcmp(msg_from_client_queue.destination,user[j]))
                        apid=user_pid[j];
                }
                FILE *fp=fopen("sgxx.txt","wb");
                fprintf(fp,"%s\n",msg_from_client_queue.destination);
                fclose(fp);
                client_msgid = msgget((key_t)apid,0666|IPC_CREAT);
                if(client_msgid == -1)
                {
                    exit(EXIT_FAILURE);
                }

                sprintf(msg_to_client_queue.original,"%s",msg_from_client_queue.original);
                sprintf(msg_to_client_queue.message,"%s",msg_from_client_queue.message);
                if(msgsnd(client_msgid, (void*)&msg_to_client_queue, sizeof(Comm_struct), 0) == -1)  
                {  
                    exit(EXIT_FAILURE);  
                }  


            }

            continue;
            res = read(fifo_ser,&msg_from_client,sizeof(Comm_struct));

            FILE *fp=fopen("msg.txt","wb");
            {
                fprintf(fp,"%s\t%s\t%s\n",msg_from_client.original,msg_from_client.destination,msg_from_client.message);
            }
            fclose(fp);
            char destination_fifo[200];
            sprintf(destination_fifo,"%s%s",MESSAGE_CLIENT_FIFO,msg_from_client.destination);
            if(!strcmp(msg_from_client.destination,"#BC"))
            {
                for(int i=0;i<user_count;++i)
                {
                    sprintf(destination_fifo,"%s%s",MESSAGE_CLIENT_FIFO,user[i]);

                    int fd = open(destination_fifo,O_WRONLY|O_NONBLOCK);
                    if(fd!=-1)
                    {
                        write(fd,&msg_from_client,sizeof(Comm_struct));
                        close(fd);
                    }
                }
                continue;
            }

            if(!strcmp(msg_from_client.destination,"-u"))
            {
                Notification do_not_exist;
                char tp_fifo[100];
                sprintf(tp_fifo,"%s%s",NOTIFICATION_FROM_SERVER,msg_from_client.original);
                int tp_fifo_fd =  open(tp_fifo,O_WRONLY|O_NONBLOCK);
                sprintf(do_not_exist.userName,"%s",msg_from_client.destination);
                //noti.log_stat = info.status;
                do_not_exist.noti_type=3;
                do_not_exist.user_count=user_count;
                memcpy(do_not_exist.allusr,user,sizeof(user));
                memcpy(do_not_exist.user_stat,user_status,sizeof(user_status));
                do_not_exist.online_user_num=online_user_num;
                write(tp_fifo_fd,&do_not_exist,sizeof(Notification));
                close(tp_fifo_fd);
                continue;

            }
            //用户不存在
            int usr_exist=0;
            for(int i=0;i<user_count;++i)
            {
                if(!strcmp(msg_from_client.destination,user[i]))
                {
                    usr_exist=1;
                    break;
                }
            }
            if(usr_exist==0)
            {
                Notification do_not_exist;
                char tp_fifo[100];
                sprintf(tp_fifo,"%s%s",NOTIFICATION_FROM_SERVER,msg_from_client.original);
                int tp_fifo_fd =  open(tp_fifo,O_WRONLY|O_NONBLOCK);
                sprintf(do_not_exist.userName,"%s",msg_from_client.destination);
                //noti.log_stat = info.status;
                do_not_exist.noti_type=2;
                write(tp_fifo_fd,&do_not_exist,sizeof(Notification));
                close(tp_fifo_fd);


            }
            int fd = open(destination_fifo,O_WRONLY|O_NONBLOCK);
            if(fd!=-1)
            {
                write(fd,&msg_from_client,sizeof(Comm_struct));
                close(fd);
            }

        }
        close(fifo_ser);

    }
    if(!strcmp("stop",argv[1]))
    {
        if (access(SERVER_MESSAGE_FIFO,F_OK) == -1)
        {
            printf("Server Not RUNNING\n");
            exit(0);
        }


        if (access(SERVER_STAT_BACK,F_OK) == -1)
        {
            res = mkfifo(SERVER_STAT_BACK,0777);
            if(res != 0 )
            {
                printf("FIFO %s was not created\n",SERVER_STAT_BACK);
                exit(EXIT_FAILURE);
            }
            printf("Stopped\n");
        }

        handler(0);

    }
    if(!strcmp("status",argv[1]))
    {
        printf("Status: ");
        if (access(SERVER_MESSAGE_FIFO,F_OK) == -1)
        {
            printf("Not RUNNING\n");
            exit(0);
        }
        else
        {
            printf("RUNNING\n");
            exit(0);
        }
    }
    return 0;
}
