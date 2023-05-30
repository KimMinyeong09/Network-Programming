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
char net[BUF_SIZE] = {0, };

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

int display(void *buf, int bytes)
{	int i;
	struct iphdr *ip = buf;
	struct icmphdr *icmp = buf+ip->ihl*4;
	struct in_addr addr;
    char *msg = (void*)icmp+8;
    int opCount;
    int operand[BUF_SIZE];
    char operator[BUF_SIZE];

	addr.s_addr = ip->saddr;

	printf("IPv%d: hdr-size=%d pkt-size=%d protocol=%d TTL=%d src=%s ",
		ip->version, ip->ihl*4, ntohs(ip->tot_len), ip->protocol,
		ip->ttl, inet_ntoa(addr));

	addr.s_addr = ip->daddr;

	printf("dst=%s type=%d \n", inet_ntoa(addr), icmp->type);
    
    if (icmp->type == 20) {
        printf("ICMP CALC: id[%d] seq[%d]\n", icmp->un.echo.id, icmp->un.echo.sequence);

        memset(net,0,sizeof(net));
        strcpy(net, inet_ntoa(addr));

        opCount = msg[0];

        for (int i=0; i<opCount; i++) { 
            int temp[4];
            temp[0] = (msg[i*4+4] & 0xff) << 24;
            temp[1] = (msg[i*4+3] & 0xff) << 16;
            temp[2] = (msg[i*4+2] & 0xff) << 8;
            temp[3] = (msg[i*4+1] & 0xff);
            operand[i] = (temp[0] | temp[1] | temp[2] | temp[3]);
        }
    
        for (int j=0; j<opCount-1; j++) {
            operator[j] = msg[(opCount*4)+j+1];
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
        return result;
    }
    else
        return -1;

}

int main(int count, char *strings[])
{	struct hostent *hname;
	struct sockaddr_in addr, cliaddr, servaddr;  // RAW용, UDP용
    int sd, sockfd, cSockfd;  // RAW용, UDP용
	unsigned char buf[BUF_SIZE];
    int result;

	if ( count != 1 )
	{
		printf("usage: %s \n", strings[0]);
		exit(0);
	}

	if ( count > 0 )
	{
		pid = getpid();  // 프로세스 아이디 얻음
		proto = getprotobyname("ICMP");  // ICMP 프로토콜 정보를 protoent 구조체에 저장

        /* RAW 소켓 생성 */
        sd = socket(PF_INET, SOCK_RAW, proto->p_proto);
        if ( sd < 0 )
        {
            perror("socket");
            exit(0);
        }

        /* UDP 소켓 생성 */
        if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { // UDP: socket type = SOCK_DGRAM
            perror("socket creation failed");
            return -1;
        }

        /* ICMP 메세지 받으면 */
        for (;;)
        {	int bytes, len=sizeof(addr);

            bzero(buf, sizeof(buf));  // buf 초기화
            bytes = recvfrom(sd, buf, sizeof(buf), 0, (struct sockaddr*)&addr, &len);
            if ( bytes > 0 ) {
                //printf("***Got message!***\n");
                result = display(buf, bytes);
                if (result > 0){
                    servaddr.sin_family = AF_INET;
                    servaddr.sin_addr.s_addr = inet_addr(net);
                    servaddr.sin_port = htons(atoi("8000"));

                    sendto(sockfd, &result, 4, 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
                }
            }
            else
                perror("recvfrom");
        }
        exit(0);

	}
	else
		printf("usage: myping <hostname>\n");
	return 0;
}