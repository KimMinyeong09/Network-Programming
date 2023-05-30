#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define BUF_SIZE 1028

int main(int argc, char **argv)
{
    int sockfd;
    int cln_send_sock, cln_rec_sock;
    struct sockaddr_in servaddr;
    struct sockaddr_in cln_broad_adr;
    struct sockaddr_in cln_adr;
    int so_brd = 1;
    int opCount, opResult;
    char buf[BUF_SIZE];

    char client[] = "client";

    char cln_buf[6] = {
        0,
    };

    if (argc != 1)
    {
        printf("Usage : ./client \n");
        return 1;
    }

    printf("Start to find calc server\n");

    cln_send_sock = socket(PF_INET, SOCK_DGRAM, 0); // UDP 소켓(Broadcast로 보낼 소켓) 생성 - server discovery 8080으로 Broadcast 보낼거임
    memset(&cln_broad_adr, 0, sizeof(cln_broad_adr));
    cln_broad_adr.sin_family = AF_INET;
    cln_broad_adr.sin_addr.s_addr = inet_addr("255.255.255.255"); // 주소
    cln_broad_adr.sin_port = htons(atoi("8080"));                 // port 번호:8081

    setsockopt(cln_send_sock, SOL_SOCKET, SO_BROADCAST, (void *)&so_brd, sizeof(so_brd)); // Broadcast로 설정

    sendto(cln_send_sock, client, strlen(client), 0, (struct sockaddr *)&cln_broad_adr, sizeof(cln_broad_adr)); // client 전송


    cln_rec_sock = socket(PF_INET, SOCK_DGRAM, 0); // UDP 소켓(Broadcast로 받을 소켓) 생성 - discovery 8082에서 메세지 받음
    memset(&cln_adr, 0, sizeof(cln_adr));
    cln_adr.sin_family = AF_INET;
    cln_adr.sin_addr.s_addr = htonl(INADDR_ANY); // 주소
    cln_adr.sin_port = htons(atoi("8082"));      // port 번호:8082

    bind(cln_rec_sock, (struct sockaddr *)&cln_adr, sizeof(cln_adr));

    recvfrom(cln_rec_sock, cln_buf, sizeof(cln_buf), 0, NULL, 0); // cal_buf: port num or fail

    if (strcmp(cln_buf, "fail") == 0)
    {
        printf("Fail\n");
        return 0;
    }
    else
    {
        printf("Found calc server(%s) \n", cln_buf);
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(atoi(cln_buf));
        servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

        connect(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr));

        printf("Operand count: ");
        scanf("%d", &opCount); // 표준 입력으로 operand count 입력받음
        if ((char)opCount <= 0)
        { // char 기준으로 opCount 값이 0보다 작거나 같을 경우(오버플로우)
            close(sockfd);
            return 0;
        }
        buf[0] = (char)opCount;

        for (int i = 0; i < opCount; i++)
        { // operand count만큼 operand 입력받기
            printf("Operand %d: ", i);
            scanf("%d", (int *)&buf[(i * 4) + 1]);
        }

        for (int j = 0; j < opCount - 1; j++)
        { // operand count-1만큼 operator 입력받기
            printf("Operator %d: ", j);
            scanf(" %c", &buf[(opCount * 4) + j + 1]);
        }

        write(sockfd, buf, 1 + (opCount * 4) + (opCount - 1)); // 서버로 char 배열 한번에 전송

        read(sockfd, &opResult, 4);                 // 결과 받기
        printf("Operation result: %d\n", opResult); // 결과 출력

        close(sockfd); // 소켓 닫기
    }

    return 0;
}