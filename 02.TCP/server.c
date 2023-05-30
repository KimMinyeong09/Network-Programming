#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char** argv) {
    int sockfd, cSockfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    char opCount;
    int operand[1024];
    char operator[1024];

    if(argc < 2) {
        printf("usage:./server localPort\n");
        return -1;
    }

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        return -1;
    }

    int enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        return -1;
    }

    if(listen(sockfd, 5) < 0) {
        perror("socket failed");
        return -1;
    }

    while(1) {
        if((cSockfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len)) < 0) {
            perror("accept error");
            return -1;
        }

        read(cSockfd, (char*)&opCount, 1);
        if (opCount <= 0) {
            break;
        }
        printf("Operand count: %d\n", opCount);

        for (int i=0; i<opCount; i++) {
            read(cSockfd, &operand[i], 4);
            printf("Operand %d: %d\n", i, operand[i]);
        }
    
        for (int j=0; j<opCount-1; j++) {
            read(cSockfd, &operator[j], 1);
            printf("Operator %d: %c\n", j, operator[j]);
        }

        int operandIndex = 0;
        int result = operand[operandIndex++];
        for (int k=0; k<opCount-1; k++) {
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

        printf("Operation result: %d\n", result);
        write(cSockfd, &result, 4);
    }
    
    printf("Server close(%d)\n", opCount);
    close(cSockfd);
    close(sockfd);
    return 0;
}