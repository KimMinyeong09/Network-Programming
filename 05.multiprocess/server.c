#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#define BUF_SIZE 1028

void read_childproc(int sig)
{
    pid_t pid;
    int status;
    pid = waitpid(-1, &status, WNOHANG);
    printf("removed proc id: %d \n", pid);
}

int main(int argc, char **argv)
{
    int sockfd, cSockfd;
    struct sockaddr_in servaddr, cliaddr;

    pid_t pid;
    struct sigaction act;
    socklen_t len;
    int length, state;
    char buf[BUF_SIZE];
    char msgbuf[BUF_SIZE];
    int operand[BUF_SIZE];
    char operator[BUF_SIZE];

    FILE *fd;
    int fds[2];
    char opCount;
    int c_pid;
    char temp[BUF_SIZE];

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

    act.sa_handler = read_childproc;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    state = sigaction(SIGCHLD, &act, 0); // child 종료되면 act 함수 호출

    if (listen(sockfd, 5) < 0)
    {
        perror("socket failed");
        return -1;
    }

    pipe(fds);
    pid = fork();
    if (pid == 0)
    { // txt 파일 사용
        fd = fopen("log.txt", "wt");
        while (1)
        {
            length = read(fds[0], msgbuf, BUF_SIZE);
            if (msgbuf[0] <= 0)
            {
                fclose(fd);
                return 0;
            }
            // printf("recieve success %s \n", msgbuf);
            fwrite((void *)msgbuf, 1, length, fd);
            //  printf("write success \n");
        }
    }

    while (1)
    {
        if ((cSockfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len)) < 0) // connect 요청 받음
        {
            continue;
        }
        else
        {
            puts("new client connected..."); // client 요청 받음
        }

        pid = fork();
        if (pid == -1) // fork error
        {
            close(cSockfd);
            continue;
        }

        if (pid == 0)
        { // child: 계산, 결과 클라이언테한테 보내주고 표준출력, 파이프를 통해 앞에서 만든 chile한테 자신의 process id와 같이 전달 후 txt에 저장
            close(sockfd);

            read(cSockfd, (char *)&opCount, 1);
            if (opCount <= 0)
            {
                printf("Save file(%d) \n", opCount);
                write(fds[1], &opCount, 1);
            }

            else
            {
                for (int i = 0; i < opCount; i++)
                {
                    read(cSockfd, &operand[i], 4);
                }

                for (int j = 0; j < opCount - 1; j++)
                {
                    read(cSockfd, &operator[j], 1);
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

                // printf("Operation result: %d\n", result);
                write(cSockfd, &result, 4);

                c_pid = getpid();

                /* buf 조합하기 */
                sprintf(buf, "%d", c_pid);
                strcat(buf, ": ");
                for (int i = 0; i < opCount - 1; i++)
                {
                    sprintf(temp, "%d", operand[i]);
                    strcat(buf, temp);
                    sprintf(temp, "%c", operator[i]);
                    strcat(buf, temp);
                }
                sprintf(temp, "%d", operand[opCount - 1]);
                strcat(buf, temp);
                strcat(buf, "=");
                sprintf(temp, "%d", result);
                strcat(buf, temp);
                strcat(buf, "\n");

                write(fds[1], buf, strlen(buf)); // 파이프 write
                printf("%s", buf);

                close(cSockfd);
                return 0;
            }
        }

        else
        { // parent
            close(cSockfd);
        }
    }

    close(sockfd);
    return 0;
}