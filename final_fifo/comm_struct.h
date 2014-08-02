/* comm_struce.h*/
#ifndef _COMM_STRUCT_H
#define _COMM_STRUCT_H
typedef struct 
{
    char original[100];     /* sender */
    char destination[100];   /* reciever */
    char message[200];        /* message */
    int type;

}   Comm_struct, *PComm_struct;

typedef struct
{
    char userName[20];
    char passWord[20];
    int status;
    int client_pid;
    char log_check_fifo[100];
    int msg_type;
} Client_info,*PClient_info;

typedef struct
{
    int noti_type;
    int user_count;
    char userName[100];
    int log_stat;
    char allusr[100][20];
    int user_stat[100];
    char other_info[2];
    int online_user_num;
} Notification,*PNotification;
#endif
