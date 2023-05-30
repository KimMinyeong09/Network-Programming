#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/uio.h>

#define MAX_CLNT 256
#define BUF_SIZE 1028
#define NAME_SIZE 10

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;

void send_msg(char *msg, int len) // 모든 클라이언트에게 메세지 보내기
{
    int i;
    pthread_mutex_lock(&mutx);
    for (i = 0; i < clnt_cnt; i++)
        write(clnt_socks[i], msg, len);
    pthread_mutex_unlock(&mutx);
}

void *handle_clnt(void *arg)
{
    int clnt_sock = *((int *)arg);
    int str_len = 0, i, recv_len;
    char msg[BUF_SIZE] = {0, };
    struct iovec vec[2];
    char Calc_data[BUF_SIZE] = {
        0,
    };
    char ID[NAME_SIZE] = {
        0,
    };

    int opCount;
    int operand[BUF_SIZE];
    char operator[BUF_SIZE];

    vec[0].iov_base = ID;
    vec[0].iov_len = 6;
    vec[1].iov_base = Calc_data;
    vec[1].iov_len = BUF_SIZE;

    while (recv_len = readv(clnt_sock, vec, 2) != 0)
    {
        memset(msg, 0, sizeof(msg));
        opCount = Calc_data[0];

        for (int i = 0; i < opCount; i++)
        {
            int temp2[4];
            temp2[0] = (Calc_data[i * 4 + 4] & 0xff) << 24;
            temp2[1] = (Calc_data[i * 4 + 3] & 0xff) << 16;
            temp2[2] = (Calc_data[i * 4 + 2] & 0xff) << 8;
            temp2[3] = (Calc_data[i * 4 + 1] & 0xff);
            operand[i] = (temp2[0] | temp2[1] | temp2[2] | temp2[3]);
        }

        for (int j = 0; j < opCount - 1; j++)
        {
            operator[j] = Calc_data[(opCount * 4) + j + 1];
        }

        int operandIndex = 0;
        int result = operand[operandIndex++];
        for (int k = 0; k < opCount - 1; k++)
        {
            switch (operator[k])
            {
            case '+':
                result += operand[operandIndex++];
                break;
            case '-':
                result -= operand[operandIndex++];
                break;
            case '*':
                result *= operand[operandIndex++];
                break;
            }
        }

        strcat(msg, ID);
        strcat(msg, " ");
        for (int l = 0; l < opCount - 1; l++)
        {
            char temp1[BUF_SIZE] = {0, };
            sprintf(temp1, "%d%c", operand[l], operator[l]);
            strcat(msg, temp1);
        }
        char temp3[BUF_SIZE] = {0, };
        sprintf(temp3, "%d=%d", operand[opCount - 1], result);
        strcat(msg, temp3);
        msg[strlen(msg)] = '\n';

        send_msg(msg, strlen(msg)); // 모든 클라이언트에게 메세지 보냄
    }

    pthread_mutex_lock(&mutx);
    for (i = 0; i < clnt_cnt; i++)
    {
        if (clnt_sock == clnt_socks[i])
        {
            while (i++ < clnt_cnt - 1)
                clnt_socks[i] = clnt_socks[i + 1];
            break;
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutx);
    close(clnt_sock);
    printf("Closed client\n");
    return NULL;
}

int main(int argc, char **argv)
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz;
    pthread_t t_id;

    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    pthread_mutex_init(&mutx, NULL);
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        printf("bind error\n");
        return 1;
    }

    int enable = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    if (listen(serv_sock, 5) == -1)
    {
        printf("listen error\n");
        return 1;
    }

    while (1)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);

        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_cnt++] = clnt_sock;
        pthread_mutex_unlock(&mutx);

        pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock);
        pthread_detach(t_id);
        printf("Connected client Port: %d \n", clnt_adr.sin_port);
    }
    close(serv_sock);
    return 0;
}