#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char** argv) {
    int sockfd, cSockfd;
    struct sockaddr_in servaddr, cliaddr;
    int len;
    char operation_data[1024];
    char opCount;
    int operand[1024];
    char operator[1024];
    int temp[4];

    if(argc < 2) {
        printf("usage:./server localPort\n");
        return -1;
    }

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { // UDP: socket type = SOCK_DGRAM
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

    while(1) {
        len = sizeof(cliaddr);
        recvfrom(sockfd, operation_data, sizeof(operation_data), 0,(struct sockaddr*)&cliaddr, &len); // 연산 데이터 받아오기
        opCount = operation_data[0];

        if (opCount <= 0) {
            break;
        }
        printf("Operand count: %d\n", opCount);

        for (int i=0; i<opCount; i++) { 
            temp[0] = (operation_data[i*4+4] & 0xff) << 24;
            temp[1] = (operation_data[i*4+3] & 0xff) << 16;
            temp[2] = (operation_data[i*4+2] & 0xff) << 8;
            temp[3] = (operation_data[i*4+1] & 0xff);
            operand[i] = (temp[0] | temp[1] | temp[2] | temp[3]);
            printf("Operand %d: %d\n", i, operand[i]);
        }
    
        for (int j=0; j<opCount-1; j++) {
            operator[j] = operation_data[(opCount*4)+j+1];
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
        sendto(sockfd, &result, 4, 0, (struct sockaddr*)&cliaddr, len);
    }
    
    printf("Server close(%d)\n", opCount);
    //close(cSockfd);
    close(sockfd);
    return 0;
}