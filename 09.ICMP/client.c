#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>

#define BUF_SIZE 1024
#define PACKETSIZE	64
struct packet
{
	struct icmphdr hdr;
	char msg[PACKETSIZE-sizeof(struct icmphdr)];
};

int pid=-1;
struct protoent *proto=NULL;

unsigned short checksum(void *b, int len)
{	unsigned short *buf = b;
	unsigned int sum=0;
	unsigned short result;

	for ( sum = 0; len > 1; len -= 2 )
		sum += *buf++;
	if ( len == 1 )
		sum += *(unsigned char*)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}

int main(int count, char *strings[])
{	struct hostent *hname;
	struct sockaddr_in addr, servaddr;  // ICMP용, UDP용
    int sd, sockfd;   // ICMP용, UDP용
    const int val=240; // TTL 설정값
    int cnt=2;
    struct packet pckt;
    int bytes;
    char input_buf[BUF_SIZE]; 
    
	if ( count != 2 )
	{
		printf("usage: %s <ip>\n", strings[0]);
		exit(0);
	}
	if ( count > 1 )
	{
		pid = getpid();  // 프로세스 아이디 얻음
		proto = getprotobyname("ICMP");  // ICMP 프로토콜 정보를 protoent 구조체에 저장
        
        
        /* RAW 소켓 생성 */
		sd = socket(PF_INET, SOCK_RAW, proto->p_proto);  // RAW 소켓 생성
        if ( sd < 0 )
        {
            perror("socket");
            return -1;
        }

        //hname = gethostbyname(strings[1]);
		bzero(&addr, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = 0;
		addr.sin_addr.s_addr = inet_addr(strings[1]);

        /* UDP 소켓 생성 */
        if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { // UDP: socket type SOCK_DGRAM
            perror("socket creation failed");
            return -1;
        }
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(atoi("8000"));
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

        if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
            perror("bind failed");
            return -1;
        }

        if (setsockopt(sd, SOL_IP, IP_TTL, &val, sizeof(val)) != 0)  // TTL 설정: 240
            perror("Set TTL option");
        if (fcntl(sd, F_SETFL, O_NONBLOCK) != 0)  // non-blocking 소켓 설정
            perror("Request nonblocking I/O");

        for (;;)
        {	int len;
            int ptr_i = 0;
            int opCount, opResult;

            //printf("Msg #%d\n", cnt);
            bzero(&pckt, sizeof(pckt));
            pckt.hdr.type = 20;
            pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
            pckt.hdr.un.echo.id = pid;
            pckt.hdr.un.echo.sequence = cnt;
            cnt += 2;

            /* calc 메세지 정리 및 전달 */
            memset(input_buf, 0, sizeof(input_buf));
            fgets(input_buf, BUF_SIZE, stdin);  // 계산식 입력받기
            input_buf[strlen(input_buf)-1] = '\0';

            char *input_list[BUF_SIZE];  // 입력받은 문자 쪼개서 저장할 리스트
            char *ptr = strtok(input_buf, " "); // 공백 문자열을 기준으로 문자열을 자름

            while (ptr != NULL) // 문자열을 자른 뒤 list에 저장
            {
                input_list[ptr_i] = ptr;
                ptr_i++;
                ptr = strtok(NULL, " ");
            }

            opCount = atoi(input_list[0]); // opCount 저장

            if ((char)opCount <= 0)
            { // char 기준으로 opCount 값이 0보다 작거나 같을 경우(오버플로우)
                close(sockfd);
                close(sd);
                printf("Overflow Number(%d) - Closed client\n", (char)opCount);
                exit(0);
            }

            pckt.msg[0] = (char)opCount;

            for (int i = 0; i < opCount; i++)
            { // operand count만큼 operand 입력받기
                int temp = atoi(input_list[i + 1]);
                pckt.msg[(i * 4) + 1] = (temp & 0xff);
                pckt.msg[(i * 4) + 2] = ((temp >> 8) & 0xff);
                pckt.msg[(i * 4) + 3] = ((temp >> 16) & 0xff);
                pckt.msg[(i * 4) + 4] = ((temp >> 24) & 0xff);
            }

            for (int j = 0; j < opCount - 1; j++)
            { // operand count-1만큼 operator 입력받기
                char temp = input_list[j + opCount + 1][0];
                pckt.msg[(opCount * 4) + j + 1] = temp;
            }
            
            if ( sendto(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&addr, sizeof(addr)) <= 0 )  // ICMP 메세지 전송
                perror("sendto");

            memset(&opResult, 0, sizeof(opResult));
            len = sizeof(servaddr);
            recvfrom(sockfd, &opResult, 4, 0, (struct sockaddr*)&servaddr, &len);
            printf("Result: %d\n", opResult); // 결과 출력    

        }
	}
	else
		printf("usage: myping <hostname>\n");
	return 0;
}