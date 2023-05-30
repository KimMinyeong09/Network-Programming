#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/uio.h>
#define BUF_SIZE 1024

void read_childproc(int sig)
{
    pid_t pid;
    int status;
    pid = waitpid(-1, &status, WNOHANG);
    printf("removed proc id: %d \n", pid);
}

int main(int argc, char **argv)
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    struct timeval timeout;
    fd_set reads, cpy_reads;

    socklen_t adr_sz;
    int fd_max, str_len, fd_num, i;

    pid_t pid;
    int fds1[2], fds2[2];
    struct sigaction act;
    int state, length_id, length_return;

    struct iovec vec[3];
    int opCount;
    int operand[BUF_SIZE];
    char operator[BUF_SIZE];
    char temp1[BUF_SIZE];
    int temp2[4];
    char data[BUF_SIZE][BUF_SIZE] = {0,};
    int data_count = 0;
    char id[4] = {0,};
    char return_nope[] = "Not exist\n";


    if (argc < 2)
    {
        printf("usage:./server localPort\n");
        return 1;
    }

    pipe(fds1), pipe(fds2);
    pid = fork();
    if (pid == 0) // child: 데이터 받을때까지 대기, mode quit일때까지는 계속 데이터 받는 상태로 대기하기
    {
        while(1){
            char idbuf[BUF_SIZE] = {0,};
            char all_data[BUF_SIZE] = {0,};
            length_id = read(fds1[0], idbuf, BUF_SIZE); 
            if (strncmp(idbuf, "quit", 4)==0){
                return 0;
            }
            else if (strncmp(idbuf, "load", 4) == 0){ // idbuf => load: aaaa
                char test_id[5] = {0, };
                for (int h = 0; h<4; h++){
                    test_id[h] = idbuf[6+h];
                }
                strcpy(all_data, "");
                for (int k = 0; k<data_count; k++){
                    if(strncmp(test_id, data[k], 4) == 0) {
                        strcat(all_data, data[k]);
                        strcat(all_data, "\n");
                    }
                }
                if (strlen(all_data))
                    write(fds2[1], all_data, strlen(all_data));
                else
                    write(fds2[1], return_nope, strlen(return_nope));
            }
            else{ // save인 경우
                strcpy(data[data_count++], idbuf);
            }
        }
        
    }
    else // parent
    {
        if ((serv_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) // TCP 소켓 생성
        {
            perror("socket creation failed");
            return -1;
        }

        serv_adr.sin_family = AF_INET;
        serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_adr.sin_port = htons(atoi(argv[1]));

        if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) < 0) // 소켓 바인드
        {
            perror("bind failed");
            return -1;
        }

        int enable = 1;
        setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)); // SO_REUSEADDR 옵션 설정

        act.sa_handler = read_childproc;
        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;
        state = sigaction(SIGCHLD, &act, 0); // child 종료되면 act 함수 호출

        if (listen(serv_sock, 5) < 0)
        {
            perror("socket failed");
            return -1;
        }

        FD_ZERO(&reads);
        FD_SET(serv_sock, &reads);
        fd_max = serv_sock;

        while (1)
        {
            cpy_reads = reads;
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;

            if ((fd_num = select(fd_max + 1, &cpy_reads, 0, 0, &timeout)) == -1)
                break;

            if (fd_num == 0) // timeout
                continue;

            for (i = 0; i < fd_max + 1; i++)
            {
                if (FD_ISSET(i, &cpy_reads))
                {
                    if (i == serv_sock)
                    { // server socket
                        adr_sz = sizeof(clnt_adr);
                        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &adr_sz);
                        FD_SET(clnt_sock, &reads);
                        if (fd_max < clnt_sock)
                            fd_max = clnt_sock;
                        printf("connected client: %d \n", clnt_sock); // client 연결
                    }

                    else
                    { // client socket
                        char mode[BUF_SIZE] = {0,};
                        char ID[BUF_SIZE] = {0,};
                        char Calc_data[BUF_SIZE] = {0,};
                        char buf[BUF_SIZE] = {0,};
                        char return_str[BUF_SIZE] = {0, };
                        
                        strcpy(mode, "");
                        strcpy(ID, "");
                        strcpy(Calc_data, "");
                        strcpy(buf, "");
                    
                        vec[0].iov_base = mode;
                        vec[0].iov_len = 4;
                        vec[1].iov_base = ID;
                        vec[1].iov_len = 4;
                        vec[2].iov_base = Calc_data;
                        vec[2].iov_len = BUF_SIZE;

                        str_len = readv(i, vec, 3);

                        if (str_len == 0)
                        { // client close 상태
                            FD_CLR(i, &reads);
                            close(i);
                            printf("closed client: %d \n", i);
                        }
                        else
                        { // client가 echo massage 보냄
                            if (strcmp(mode, "save") == 0)
                            {
                                printf("save to %s \n", ID);

                                opCount = Calc_data[0];

                                for (int l = 0; l < opCount; l++)
                                {
                                    temp2[0] = (Calc_data[l * 4 + 4] & 0xff) << 24;
                                    temp2[1] = (Calc_data[l * 4 + 3] & 0xff) << 16;
                                    temp2[2] = (Calc_data[l * 4 + 2] & 0xff) << 8;
                                    temp2[3] = (Calc_data[l * 4 + 1] & 0xff);
                                    operand[l] = (temp2[0] | temp2[1] | temp2[2] | temp2[3]);
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

                                write(clnt_sock, &result, 4);

                                strcat(buf, ID);
                                strcat(buf,": ");
                                for (int l = 0; l < opCount - 1; l++)
                                {
                                    sprintf(temp1, "%d", operand[l]);
                                    strcat(buf, temp1);
                                    sprintf(temp1, "%c", operator[l]);
                                    strcat(buf, temp1);
                                }
                                sprintf(temp1, "%d", operand[opCount - 1]);
                                strcat(buf, temp1);
                                strcat(buf, "=");
                                sprintf(temp1, "%d", result);
                                strcat(buf, temp1);

                                write(fds1[1], buf, strlen(buf)); // 파이프 write
                                
                            }
                            else if (strcmp(mode, "load") == 0) // pipe에서 ID에 해당하는 데이터 꺼내오기
                            {
                                printf("load from %s \n", ID);
                                strcat(buf, mode);
                                strcat(buf,": ");
                                strcat(buf, ID);
                                write(fds1[1], buf, strlen(buf));
                                length_return = read(fds2[0], return_str, BUF_SIZE);
                                write(i, return_str, length_return);
                            }
                            else if (strcmp(mode, "quit") == 0)// 자식(pipe 프로세스한테 quit mode pipe로 전송하고) 종료, 부모도 종료 
                            {
                                printf("quit\n");
                                strcat(buf, mode);
                                write(fds1[1], buf, strlen(buf));
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    close(serv_sock);

    return 0;
}