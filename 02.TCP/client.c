#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char** argv) {
    int sockfd;
    char buf[1024];
    char *id = "2019113389";
    struct sockaddr_in servaddr;
    socklen_t len;
    int opCount, opResult;

    if(argc < 3) {
        printf("usage:./client remoteAddress remotePort\n");
        return -1;
    }

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        return -1;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[1]));
    servaddr.sin_addr.s_addr = inet_addr(argv[2]);
    
    if(connect(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect error");
        return -1;
    }

    printf("Operand count: ");
    scanf("%d", &opCount); // 표준 입력으로 operand count 입력받음
    if ((char)opCount <= 0) { // char 기준으로 opCount 값이 0보다 작거나 같을 경우(오버플로우)
        write(sockfd, &opCount, 1);
        close(sockfd);
        return 0;
    }
    buf[0] = (char)opCount;

    for (int i = 0; i<opCount; i++) { //operand count만큼 operand 입력받기
        printf("Operand %d: ", i);
        scanf("%d", (int*)&buf[(i*4)+1]);
    }

    for (int j = 0; j<opCount-1; j++) { //operand count-1만큼 operator 입력받기
        printf("Operator %d: ", j);
        scanf(" %c", &buf[(opCount*4)+j+1]);
    }

    write(sockfd, buf, 1+(opCount*4)+(opCount-1)); // 서버로 char 배열 한번에 전송

    read(sockfd, &opResult, 4); // 결과 받기
    printf("Operation result: %d\n", opResult); // 결과 출력

    close(sockfd); // 소켓 닫기
    return 0;
}