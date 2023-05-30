#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define BUF_SIZE 1024

int main(int argc, char **argv)
{
    int sockfd, cSockfd;
    FILE *fd;
    int length;
    char buf[BUF_SIZE];
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;

    if (argc < 2)
    {
        printf("usage:./server localPort\n");
        return 1;
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) // TCP 소켓 생성
    {
        perror("socket creation failed");
        return -1;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) // 소켓 바인드
    {
        perror("bind failed");
        return -1;
    }

    int enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)); // SO_REUSEADDR 옵션 설정

    if ((fd = fopen("text.txt", "r")) < 0) // text.txt 읽기 권한으로 열기
    {
        perror("open failed");
        return -1;
    };

    if (listen(sockfd, 5) < 0)
    {
        perror("socket failed");
        return -1;
    }

    if ((cSockfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len)) < 0) // connect 요청 받음
    {
        perror("accept error");
        return -1;
    }

    fseek(fd, 0, SEEK_SET);

    while (1) // 파일 데이터 전송
    {
        length = fread((void *)buf, 1, BUF_SIZE, fd);
        if (length < BUF_SIZE)
        {
            write(cSockfd, buf, length);
            break;
        }
        write(cSockfd, buf, BUF_SIZE);
    }

    // Half close 수행: Write Buffer 닫기
    shutdown(cSockfd, SHUT_WR);

    // 클라이언트 종료 전에 보내는 파일 데이터를 모두 받고 출력
    printf("Message from Client\n");
    while (1)
    {
        length = read(cSockfd, buf, BUF_SIZE);
        if (length < BUF_SIZE)
        {
            printf("%s \n", buf);
            break;
        }
        printf("%s", buf);
    }

    fclose(fd);
    close(cSockfd);
    close(sockfd);
    return 0;
}