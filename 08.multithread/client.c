#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/uio.h>

#define BUF_SIZE 1028
#define NAME_SIZE 10


char name[NAME_SIZE] = {
    0,
};
char msg[BUF_SIZE];


void *send_msg(void *arg)
{
    int sock = *((int *)arg);
    char input_buf[BUF_SIZE]; // 계산식 입력받는 곳
    char buf[BUF_SIZE];       // 계산식을 바이트 수에 맞춰서 저장할 공간
    struct iovec vec[3];
    int write_len;

    while (1)
    {
        int ptr_i = 0;
        int opCount, opResult;

        memset(input_buf, 0, sizeof(input_buf));
        memset(buf, 0, sizeof(buf));

        fgets(input_buf, BUF_SIZE, stdin);
        input_buf[strlen(input_buf)-1] = '\0';

        char *input_list[BUF_SIZE];  // 입력받은 문자 쪼개서 리스트로 저장
        char *ptr = strtok(input_buf, " "); // 공백 문자열을 기준으로 문자열을 자름

        while (ptr != NULL) // 문자열을 자른 뒤 temp에 저장
        {
            input_list[ptr_i] = ptr;
            ptr_i++;
            ptr = strtok(NULL, " ");
        }

        opCount = atoi(input_list[0]);

        if ((char)opCount <= 0)
        { // char 기준으로 opCount 값이 0보다 작거나 같을 경우(오버플로우)
            close(sock);
            printf("Overflow Number(%d) - Closed client\n", (char)opCount);
            exit(0);
        }

        buf[0] = (char)opCount;

        for (int i = 0; i < opCount; i++)
        { // operand count만큼 operand 입력받기
            int temp = atoi(input_list[i + 1]);
            buf[(i * 4) + 1] = (temp & 0xff);
            buf[(i * 4) + 2] = ((temp >> 8) & 0xff);
            buf[(i * 4) + 3] = ((temp >> 16) & 0xff);
            buf[(i * 4) + 4] = ((temp >> 24) & 0xff);
        }

        for (int j = 0; j < opCount - 1; j++)
        { // operand count-1만큼 operator 입력받기
            char temp = input_list[j + opCount + 1][0];
            buf[(opCount * 4) + j + 1] = temp;
        }

        vec[0].iov_base = name;
        vec[0].iov_len = 6;
        vec[1].iov_base = buf;
        vec[1].iov_len = BUF_SIZE;

        write_len = writev(sock, vec, 2);
    }
    return NULL;
}

void *recv_msg(void *arg)
{
    int sock = *((int *)arg);
    char msg[NAME_SIZE];
    int str_len;
    while (1)
    {
        str_len = read(sock, msg,  BUF_SIZE);
        if (str_len == -1)
            return (void *)-1;
        msg[str_len] = 0;
        fputs(msg, stdout);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void *thread_return;

    if (argc != 4)
    {
        printf("Usage : %s <Port> <IP> <name>\n", argv[0]);
        exit(1);
    }

    if (strlen(argv[3]) != 4)
    {
        printf("ID have to be 4\n");
        return 0;
    }

    sprintf(name, "[%s]", argv[3]);
    sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[2]);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        printf("connect error\n");
        return 1;
    }

    pthread_create(&snd_thread, NULL, send_msg, (void *)&sock); //메세지 보냄
    pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock); // 메세지 받음
    pthread_join(snd_thread, &thread_return);
    pthread_join(rcv_thread, &thread_return);
    close(sock);

    return 0;
}