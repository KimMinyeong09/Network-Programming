#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#define BUF_SIZE 1028

int main(int argc, char **argv)
{
    int sockfd;
    int temp;
    char buf[BUF_SIZE] = {0,}, data[BUF_SIZE];
    struct sockaddr_in servaddr;
    socklen_t len;
    int opCount, opResult, str_len;

    struct iovec vec[3];
    char mode[BUF_SIZE] = {
        0,
    };
    char ID[BUF_SIZE] = {
        0,
    };
    char Calc_data[BUF_SIZE] = {
        0,
    };

    if (argc < 3)
    {
        printf("usage:./client remoteAddress remotePort\n");
        return -1;
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket creation failed");
        return -1;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[1]));
    servaddr.sin_addr.s_addr = inet_addr(argv[2]);

    printf("Mode: ");
    scanf("%s", mode);

    if (strcmp(mode, "save") == 0 || strcmp(mode, "load") == 0)
    {
        printf("ID: ");
        scanf("%s", ID);
        if (strlen(ID) != 4)
        { // ID 길이 4 넘어감
            printf("Error: ID length must be 4 \n");
            return 0;
        }
    }
    else if (strcmp(mode, "quit") != 0)
    { // mode 지원 X
        printf("supported mode: save load quit \n");
        return 0;
    }

    if (connect(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    { // server connect
        perror("connect error");
        return -1;
    }

    if (strcmp(mode, "save") == 0)
    {
        printf("Operand count: ");
        scanf("%d", &opCount); // 표준 입력으로 operand count 입력받음
        if ((char)opCount <= 0)
        { // char 기준으로 opCount 값이 0보다 작거나 같을 경우(오버플로우)
            printf("Overflow will happen(%d) \n", (char)opCount);
            close(sockfd);
            return 0;
        }

        buf[0] = (char)opCount;

        for (int i = 0; i < opCount; i++)
        { // operand count만큼 operand 입력받기
            printf("Operand %d: ", i);
            scanf("%d", &temp);
            buf[(i*4)+1] = (temp & 0xff);
            buf[(i*4)+2] = ((temp>>8) & 0xff);
            buf[(i*4)+3] = ((temp>>16) & 0xff);
            buf[(i*4)+4] = ((temp>>24) & 0xff);
        }

        for (int j = 0; j < opCount - 1; j++)
        { // operand count-1만큼 operator 입력받기
            printf("Operator %d: ", j);
            scanf(" %c", &buf[(opCount * 4) + j + 1]);
        }

        vec[0].iov_base = mode;
        vec[0].iov_len = 4;
        vec[1].iov_base = ID;
        vec[1].iov_len = 4;
        vec[2].iov_base = buf;
        vec[2].iov_len = BUF_SIZE;

        str_len = writev(sockfd, vec, 3);

        read(sockfd, &opResult, 4);                 // 결과 받기
        printf("Operation result: %d\n", opResult); // 결과 출력
    }
    else if (strcmp(mode, "load") == 0)
    {
        vec[0].iov_base = mode;
        vec[0].iov_len = 4;
        vec[1].iov_base = ID;
        vec[1].iov_len = 4;
        vec[2].iov_base = buf;
        vec[2].iov_len = BUF_SIZE;

        str_len = writev(sockfd, vec, 3);

        read(sockfd, data, BUF_SIZE);                 // 결과 받기
        printf("%s", data);
    }
    else
    {
        vec[0].iov_base = mode;
        vec[0].iov_len = 4;
        vec[1].iov_base = ID;
        vec[1].iov_len = 4;
        vec[2].iov_base = buf;
        vec[2].iov_len = BUF_SIZE;
        
        str_len = writev(sockfd, vec, 3);
    }

    close(sockfd); // 소켓 닫기
    return 0;
}