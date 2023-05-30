#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define BUF_SIZE 1028

int main(int argc, char **argv)
{
    int serv_sock, clnt_sock;
    int dis_send_sock, cal_send_sock, dis_rec_sock, cal_rec_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    struct sockaddr_in dis_broad_adr, cal_broad_adr;
    struct sockaddr_in dis_adr, cal_adr;

    int so_brd = 1;
    int random;
    char cal_to_dis[13] = {
        0,
    };

    char cal_buf[10] = {
        0,
    };
    char port_num[6] = {
        0,
    };
    char temp[6] = {
        0,
    };

    char success[] = "success";
    char fail[] = "fail";

    fd_set reads, cpy_reads;
    socklen_t adr_sz;
    struct timeval timeout;
    int fd_max, str_len, fd_num, i;
    char buf[BUF_SIZE];

    if (argc != 2)
    {
        printf("Usage : ./server <Mode> \n");
        return 1;
    }

    if (strcmp(argv[1], "discovery") == 0)
    {
        dis_rec_sock = socket(PF_INET, SOCK_DGRAM, 0); // UDP 소켓(Broadcast로 받을 소켓) 생성 - cal 8080에서 port 가져옴
        memset(&dis_adr, 0, sizeof(dis_adr));
        dis_adr.sin_family = AF_INET;
        dis_adr.sin_addr.s_addr = htonl(INADDR_ANY); // 주소
        dis_adr.sin_port = htons(atoi("8080"));      // port 번호:8080

        printf("Discovery Server operating...\n");

        bind(dis_rec_sock, (struct sockaddr *)&dis_adr, sizeof(dis_adr));
        while (1)
        {
            char dis_buf[13] = {
                0,
            };
            recvfrom(dis_rec_sock, dis_buf, sizeof(dis_buf), 0, NULL, 0);

            dis_send_sock = socket(PF_INET, SOCK_DGRAM, 0); // UDP 소켓(Broadcast로 보낼 소켓) 생성 - 8081, 8082로 Broadcast 보낼거임
            memset(&dis_broad_adr, 0, sizeof(dis_broad_adr));
            dis_broad_adr.sin_family = AF_INET;
            dis_broad_adr.sin_addr.s_addr = inet_addr("255.255.255.255"); // 주소

            if (strncmp(dis_buf, "server:", 7) == 0)
            {                                                                                         // port 번호 받은 경우
                dis_broad_adr.sin_port = htons(atoi("8081"));                                         // port 번호:8081
                setsockopt(dis_send_sock, SOL_SOCKET, SO_BROADCAST, (void *)&so_brd, sizeof(so_brd)); // Broadcast로 설정

                if (strlen(port_num) != 5)
                { // port num 저장 안되어있는 경우
                    strncpy(port_num, &(dis_buf[7]), 5);
                    printf("Calc Server registered(%s) \n", port_num);
                    sendto(dis_send_sock, success, strlen(success), 0, (struct sockaddr *)&dis_broad_adr, sizeof(dis_broad_adr)); // success 전송
                }
                else
                { // port num 이미 저장되어있는 경우
                    sendto(dis_send_sock, fail, strlen(fail), 0, (struct sockaddr *)&dis_broad_adr, sizeof(dis_broad_adr)); // fail 전송
                }
            }
            else if (strcmp(dis_buf, "client") == 0)
            { // 클라이언트가 연결된 경우
                dis_broad_adr.sin_port = htons(atoi("8082"));                                         // port 번호:8082
                setsockopt(dis_send_sock, SOL_SOCKET, SO_BROADCAST, (void *)&so_brd, sizeof(so_brd)); // Broadcast로 설정

                if (strlen(port_num) != 5)
                { // port num 저장 안되어있는 경우
                    sendto(dis_send_sock, fail, strlen(fail), 0, (struct sockaddr *)&dis_broad_adr, sizeof(dis_broad_adr)); // fail 전송

                }
                else
                { // port num 저장되어있는 경우
                    sendto(dis_send_sock, port_num, strlen(port_num), 0, (struct sockaddr *)&dis_broad_adr, sizeof(dis_broad_adr)); // port 전송
                }
            }
            //close(dis_send_sock);
        }
    }
    else if (strcmp(argv[1], "calc") == 0)
    {
        random = rand() % 40001 + 10000; // ran_port: 10000<=random<=50000
        printf("Register calc server\n");
        cal_send_sock = socket(PF_INET, SOCK_DGRAM, 0); // UDP 소켓(Broadcast로 보낼 소켓) 생성 - dis 8080으로 Broadcast 보낼거임
        memset(&cal_broad_adr, 0, sizeof(cal_broad_adr));
        cal_broad_adr.sin_family = AF_INET;
        cal_broad_adr.sin_addr.s_addr = inet_addr("255.255.255.255"); // 주소
        cal_broad_adr.sin_port = htons(atoi("8080"));                 // port 번호:8080

        setsockopt(cal_send_sock, SOL_SOCKET, SO_BROADCAST, (void *)&so_brd, sizeof(so_brd)); // Broadcast로 설정

        strcpy(cal_to_dis, "server:");
        sprintf(temp, "%d", random);
        strcat(cal_to_dis, temp);

        sendto(cal_send_sock, cal_to_dis, strlen(cal_to_dis), 0, (struct sockaddr *)&cal_broad_adr, sizeof(cal_broad_adr)); // port num 전송

        close(cal_send_sock);

        cal_rec_sock = socket(PF_INET, SOCK_DGRAM, 0); // UDP 소켓(Broadcast로 받을 소켓) 생성 - discovery 8081에서 메세지 받음
        memset(&cal_adr, 0, sizeof(cal_adr));
        cal_adr.sin_family = AF_INET;
        cal_adr.sin_addr.s_addr = htonl(INADDR_ANY); // 주소
        cal_adr.sin_port = htons(atoi("8081"));      // port 번호:8081

        bind(cal_rec_sock, (struct sockaddr *)&cal_adr, sizeof(cal_adr));
        recvfrom(cal_rec_sock, cal_buf, sizeof(cal_buf), 0, NULL, 0); // cal_buf: success or fail

        if (strcmp(cal_buf, "success") == 0)
        {
            printf("Calc Server(%d) operating...\n", random);
        }
        else if (strcmp(cal_buf, "fail") == 0)
        {
            printf("Fail\n");
            return 0;
        }
        close(cal_rec_sock);

        /* Calc 서버 */
        serv_sock = socket(PF_INET, SOCK_STREAM, 0);
        memset(&serv_adr, 0, sizeof(serv_adr));
        serv_adr.sin_family = AF_INET;
        serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_adr.sin_port = htons(random);

        if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) < 0) // 소켓 바인드
        {
            printf("bind error\n");
            return 1;
        }
        if (listen(serv_sock, 5) < 0)
        {
            printf("listen error\n");
            return 1;
        }

        FD_ZERO(&reads);
        FD_SET(serv_sock, &reads);
        fd_max = serv_sock;

        while (1)
        {
            cpy_reads = reads;
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;

            fd_num = select(fd_max + 1, &cpy_reads, 0, 0, &timeout);

            if (fd_num == 0) // timeout
                continue;

            for (i = 0; i < fd_max + 1; i++)
            {
                if (FD_ISSET(i, &cpy_reads))
                {
                    if (i == serv_sock)
                    { // server: client 연결하기
                        adr_sz = sizeof(clnt_adr);
                        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &adr_sz);
                        FD_SET(clnt_sock, &reads);
                        if (fd_max < clnt_sock)
                            fd_max = clnt_sock;
                        printf("connected client: %d \n", clnt_sock);
                    }
                    else
                    { // client: 계산 데이터 읽어오기
                        char opCount;
                        int operand[BUF_SIZE];
                        char operator[BUF_SIZE];

                        read(i, (char *)&opCount, 1); // 계산 데이터 읽기
                        for (int k = 0; k < opCount; k++)
                        {
                            read(i, &operand[k], 4);
                        }

                        for (int k = 0; k < opCount - 1; k++)
                        {
                            read(i, &operator[k], 1);
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

                        write(i, &result, 4);
                        close(i);
                        printf("closed client: %d \n", i);
                    }
                }
            }
        }
    }
    else
    {
        printf("./server discovery | ./server calc\n");
        return 0;
    }

    return 0;
}