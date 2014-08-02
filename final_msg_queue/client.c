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
#define SERVER_CLIENT_INFO "./fifo/server/clientinfo" 
#define NOTIFICATION_FROM_SERVER "./fifo/client/noti_"
#define LOG_CHECK_FIFO "./fifo/client/log_ck_"
#define MESSAGE_CLIENT_FIFO "./fifo/client/msg_"
#define LOGIN 1
#define LOGOFF 0
char toUser[20];
char my_msg_fifo[200];
char online_user[100][20];
int online_user_num=1;
char my_noti[200];
char curr_user[20];
int ser_msgid,client_msgid;
Comm_struct msg_to_server,msg_from_server;

void recieveMessage_Thread()
{
    while(msgrcv(client_msgid, (void*)&msg_from_server, sizeof(Comm_struct), 0, 0) != -1)
    {

        //  if(msg_from_server.getNew==1)
        {
            printf("\033[31mFrom %s:\033[30m %s\n",msg_from_server.original,msg_from_server.message);
            // msg_from_server->getNew=0;
        }
    }

}
void recieveNotification_Thread()
{
    Notification noti;
    int res;
    int fifo_fd = open(my_noti,O_RDONLY);
    if(fifo_fd == -1)
    {
        printf("Could not open %s for read only access\n",my_noti);
        exit(EXIT_FAILURE);
    }
    while(1)
    {
        res = read(fifo_fd,&noti,sizeof(Notification));

        if(res>0)
        {
            if(noti.noti_type==2)
            {
                printf("ERROR! %s do not exist!\n",noti.userName);
                continue;
            }
            if(noti.noti_type==3)
            {

                online_user_num=0;
                for(int i=0;i<noti.user_count;++i)
                {
                    if(noti.user_stat[i]==1)
                        online_user_num++;

                }
                printf("----User Check----\n");
                printf("Online User Number: [%d]\n",online_user_num);

                for(int i=0;i<noti.user_count;++i)
                {
                    printf("%s %s\033[30m\n",noti.allusr[i],noti.user_stat[i]==1?"\033[32mOnline":"\033[31mOff-Line");

                }

                printf("----end-----\n");
                continue;
            }
            if(noti.noti_type==1)
            {
                printf("\033[30m%s : %s \033[30m\n",noti.userName,noti.log_stat==0?"\033[31mOff-Line":"\033[32mOnline");
            }
        }


    }
    close(fifo_fd);

}
void handler(int sig)
{
    Client_info client_info;
    client_info.status=LOGOFF;
    sprintf(client_info.userName,"%s",curr_user);
    int fd1 = open(SERVER_CLIENT_INFO,O_WRONLY|O_NONBLOCK);
    write(fd1,&client_info,sizeof(Client_info));
    close(fd1);

    exit(1);
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


    ser_msgid = msgget((key_t)1111,0666|IPC_CREAT);
    if(ser_msgid == -1)
    {
        exit(EXIT_FAILURE);
    }

    client_msgid = msgget((key_t)getpid(),0666|IPC_CREAT);
    if(client_msgid == -1)
    {
        exit(EXIT_FAILURE);
    }

    //client -l userName password
    //
    sprintf(curr_user,"%s",argv[1]);
    sprintf(online_user[0],"%s",argv[1]);
    if(argc!=3)
    {
        printf("Usage: client userName passWord\n");
        exit(0);
    }

    Client_info client_info;
    int res; 

    //Login Check
    sprintf(client_info.userName,"%s",argv[1]);
    sprintf(client_info.passWord,"%s",argv[2]);
    client_info.status=LOGIN;
    client_info.client_pid=getpid();
    sprintf(client_info.log_check_fifo,"%s%d",LOG_CHECK_FIFO,getpid());

    if (access(client_info.log_check_fifo,F_OK) == -1)
    {
        res = mkfifo(client_info.log_check_fifo,0777);
        if(res != 0 )
        {
            printf("FIFO %s was not created\n",client_info.log_check_fifo);
            exit(EXIT_FAILURE);
        }
    }
    client_info.client_pid=getpid();
    int fd1 = open(SERVER_CLIENT_INFO,O_WRONLY|O_NONBLOCK);
    write(fd1,&client_info,sizeof(Client_info));
    close(fd1);


    int fifo_ck = open(client_info.log_check_fifo,O_RDONLY);
    char log_tp;
    res = read(fifo_ck,&log_tp,sizeof(char));
    if(log_tp=='0')
    {
        printf("Passwrod Error! Please Login Again! \n");
        return 0;
    }
    if(log_tp=='2')
    {
        printf("New User! Your Name:\033[31m %s \033[30m,PSW: %s \n",argv[1],argv[2]);
    }
    if(log_tp=='1')
    {
        printf("Login! Your Name: \033[31m%s\033[30m\n",argv[1]);
    }

    char usr_cmd;    

    //Create MY Client Message FIFO
    sprintf(my_msg_fifo,"%s%s",MESSAGE_CLIENT_FIFO,argv[1]);
    if (access(my_msg_fifo,F_OK) == -1)
    {
        res = mkfifo(my_msg_fifo,0777);
        if(res != 0 )
        {
            printf("FIFO %s was not created\n",my_msg_fifo);
            exit(EXIT_FAILURE);
        }
    }


    //Create CLIENT Notification FIFO
    sprintf(my_noti,"%s%s",NOTIFICATION_FROM_SERVER,argv[1]);
    if (access(my_noti,F_OK) == -1)
    {
        res = mkfifo(my_noti,0777);
        if(res != 0 )
        {
            printf("FIFO %s was not created\n",my_noti);
            exit(EXIT_FAILURE);
        }
    }

    //Open server
    int ser_fifo=open(SERVER_MESSAGE_FIFO,O_WRONLY);
    if(ser_fifo==-1)
    {
        printf("Open server ERROR\n");
        exit(EXIT_FAILURE);
    }

    char comm_msg[200];
    while(1)
    {

        pthread_t tRecieveMessage,tNotification;
        int noti_rec =  pthread_create(&tNotification,NULL,(void *)recieveNotification_Thread,NULL);
        int rec_ret = pthread_create(&tRecieveMessage,NULL,(void *)recieveMessage_Thread,NULL);

        printf("\033[40m\033[37mEnter your order:\033[49m\033[30m\n1.chat with someone[userName@Message]\n2.broadcast to all[#BC@Message]\n");
        printf("3.check all online users[-u].\n4.help[-h]\n");
        //scanf("%c",&usr_cmd);
        //getchar();
        while(1)
        {
            scanf("%[^\n]",comm_msg);
            char *toUser = strtok(comm_msg,"@");
            char *msg = strtok(NULL,"@");
            // printf("%s,%s",p1,p2);

            if(!strcmp(toUser,"-h"))
            {
                printf("\033[40m\033[37mEnter your order :\033[49m\033[30m\n1.chat with someone[userName@Message]\n2.broadcast to all[#BC@Message]\n");
                printf("3.check all online users[-u].\n4.help[-h]\n");
                getchar();
                continue;
            }

            if(!strcmp(toUser,"-u"))
            {
                //printf("OnLine User :\n ");
                // for(int i =0;i<online_user_num;++i)
                //     printf("%s \n",online_user[i]);
                // printf("----Check End----\n");
                // getchar();
                Comm_struct msg_to_server;
                sprintf(msg_to_server.original,"%s",argv[1]);
                msg_to_server.type=0;
                sprintf(msg_to_server.destination,"%s",toUser);
                sprintf(msg_to_server.message,"-u");


                write(ser_fifo,&msg_to_server,sizeof(Comm_struct));
                getchar();

                continue;

            }



            sprintf(msg_to_server.original,"%s",argv[1]);
            msg_to_server.type=0;
            sprintf(msg_to_server.destination,"%s",toUser);
            if(strcmp(toUser,"#BC"))
                sprintf(msg_to_server.message,"%s",msg);
            else
                sprintf(msg_to_server.message,"[#Broadcast] %s",msg);
            msg_to_server.getNew=1;
            if(msgsnd(ser_msgid, &msg_to_server, sizeof(Comm_struct), IPC_NOWAIT) == -1)  
            {  
                exit(EXIT_FAILURE);  
            }  
            getchar();
            //  write(ser_fifo,&msg_to_server,sizeof(Comm_struct));



        }

    }
    return 0;
}
