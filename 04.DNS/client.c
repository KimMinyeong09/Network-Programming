#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#define BUF_SIZE 100

int main(int argc, char **argv)
{
    int sockfd;
    int sock_type;
    FILE *fd;
    socklen_t optlen;
    int state;
    int length;
    char buf[BUF_SIZE];
    struct hostent *host, *host2;
    struct sockaddr_in addr;
    struct sockaddr_in servaddr;

    if (argc == 2)
    { // 매개변수로 도메인 이름 하나만
        host = gethostbyname(argv[1]);
        if (!host)
        {
            perror("gethostbyname() error");
            return 1;
        }

        printf("gethostbyname()\n");
        printf("Official name: %s \n", host->h_name);
        for (int i = 0; host->h_aliases[i]; i++)
        {
            printf("Aliases %d: %s \n", i, host->h_aliases[i]);
        }

        printf("Address type: %s \n", (host->h_addrtype == AF_INET) ? "AF_INET" : "AF_INET6");
        for (int i = 0; host->h_addr_list[i]; i++)
        {
            printf("IP addr %d: %s \n", i, inet_ntoa(*(struct in_addr *)host->h_addr_list[i]));
        }

        memset(&addr, 0, sizeof(addr));
        addr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr *)host->h_addr_list[0]));
        host2 = gethostbyaddr((char *)&addr.sin_addr, 4, AF_INET); // 첫번째 IP 정보 이용
        if (!host2)
        {
            perror("gethostbyaddr() error");
            return 1;
        }

        printf("\ngethostbyaddr()\n");
        printf("Official name: %s \n", host2->h_name);
        for (int i = 0; host2->h_aliases[i]; i++)
        {
            printf("Aliases %d: %s \n", i, host2->h_aliases[i]);
        }

        printf("Address type: %s \n", (host2->h_addrtype == AF_INET) ? "AF_INET" : "AF_INET6");
        for (int i = 0; host2->h_addr_list[i]; i++)
        {
            printf("IP addr %d: %s \n", i, inet_ntoa(*(struct in_addr *)host2->h_addr_list[i]));
        }
    }

    else if (argc == 3)
    { // 매개변수로 포트번호와 IP주소

        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) // TCP 소켓 생성
        {
            perror("socket creation failed");
            return 1;
        }

        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(atoi(argv[1]));
        servaddr.sin_addr.s_addr = inet_addr(argv[2]);

        optlen = sizeof(sock_type);
        state = getsockopt(sockfd, SOL_SOCKET, SO_TYPE, (void *)&sock_type, &optlen); // 소켓타입 정보 받기
        if (state)
        {
            perror("getsockopt() error");
            return 1;
        }

        printf("This socket type is: %d(%d) \n", sock_type, SOCK_STREAM);

        fd = fopen("copy.txt", "w+"); // copy.txt 열기, 없으면 생성

        if (connect(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) // 연결 요청
        {
            perror("connect error");
            return 1;
        }

        while ((length = read(sockfd, buf, BUF_SIZE)) != 0) // 서버에서 파일데이터 받기
        {
            fwrite((void *)buf, 1, length, fd);
        }
        printf("Received file data\n");

        // 서버 Half Close

        while (1)
        { // 파일 데이터 전송
            length = fread((void *)buf, 1, BUF_SIZE, fd);
            if (length < BUF_SIZE)
            {
                write(sockfd, buf, length);
                break;
            }
            write(sockfd, buf, BUF_SIZE);
        }

        fclose(fd);
        close(sockfd); // 소켓 닫기
    }

    else
    {
        printf("Usage: %s <DomainName> <RemoteAddress>\n", argv[0]);
        return 1;
    }

    return 0;
}