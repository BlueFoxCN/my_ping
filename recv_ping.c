
/********************************************************
 *	IP报头格式数据结构定义在<netinet/ip.h>中	*
 *	ICMP数据结构定义在<netinet/ip_icmp.h>中		*
 *	套接字地址数据结构定义在<netinet/in.h>中	*
 ********************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <setjmp.h>
#include <errno.h>

#define	PACKET_SIZE	4096
#define	MAX_WAIT_TIME	5
#define	MAX_NO_PACKETS	10000


char *addr[];
char sendpacket[PACKET_SIZE];
char recvpacket[PACKET_SIZE];
int sockfd,datalen = 56;
int nsend = 0, nreceived = 0;
double temp_rtt[MAX_NO_PACKETS];
double all_time = 0;
double min = 0;
double max = 0;
double avg = 0;
double mdev = 0;

struct sockaddr_in dest_addr;
struct sockaddr_in from;
struct timeval tvrecv;
pid_t pid;

void recv_packet(void);
void computer_rtt(void);
int unpack(char *buf,int len);

/****统计数据函数****/
void statistics(int sig)
{
	close(sockfd);
	exit(1);
}

/****接受所有ICMP报文****/
void recv_packet()
{
	int n,fromlen;
	extern int error;
	fromlen = sizeof(from);
	printf("begin to receive\n");
		//接收数据报
		if((n = recvfrom(sockfd,recvpacket,sizeof(recvpacket),0,
			(struct sockaddr *)&from,&fromlen)) < 0)
		{
			perror("recvfrom error");
		}
		gettimeofday(&tvrecv,NULL);		//记录接收时间
		unpack(recvpacket,n);		//剥去ICMP报头
		nreceived++;
		printf("receive a packet\n");
}


/******剥去ICMP报头******/
int unpack(char *buf,int len)
{
	int i;
	int iphdrlen;		//ip头长度
	struct ip *ip;
	struct icmp *icmp;
	double rtt;


	ip = (struct ip *)buf;
	iphdrlen = ip->ip_hl << 2;	//求IP报文头长度，即IP报头长度乘4
	icmp = (struct icmp *)(buf + iphdrlen);	//越过IP头，指向ICMP报头
	len -= iphdrlen;	//ICMP报头及数据报的总长度
	if(len < 8)		//小于ICMP报头的长度则不合理
	{
		printf("ICMP packet\'s length is less than 8\n");
		return -1;
	}
	//确保所接收的是所发的ICMP的回应
	if((icmp->icmp_type == ICMP_ECHOREPLY) && (icmp->icmp_id == pid))
	{
		// tvsend = (struct timeval *)icmp->icmp_data;
    char *data = (char *)icmp->icmp_data;
		//显示相关的信息
		printf("receive info: %s\n", data);
		// printf("%d bytes from %s: icmp_seq=%u ttl=%d time=%.1f ms\n",
		//		len,inet_ntoa(from.sin_addr),
		//		icmp->icmp_seq,ip->ip_ttl,rtt);
	}
	else return -1;
}


/*主函数*/
main(int argc,char *argv[])
{
	struct hostent *host;
	struct protoent *protocol;
	unsigned long inaddr = 0;
	int size = 50 * 1024;
	addr[0] = argv[1];
	//不是ICMP协议
	if((protocol = getprotobyname("icmp")) == NULL)
	{
		perror("getprotobyname");
		exit(1);
	}

	//生成使用ICMP的原始套接字，只有root才能生成
	if((sockfd = socket(AF_INET,SOCK_RAW,protocol->p_proto)) < 0)
	{
		perror("socket error");
		exit(1);
	}

	//回收root权限，设置当前权限
	setuid(getuid());

	/*扩大套接字的接收缓存区导50K，这样做是为了减小接收缓存区溢出的
	  可能性，若无意中ping一个广播地址或多播地址，将会引来大量的应答*/
	setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,&size,sizeof(size));
	bzero(&dest_addr,sizeof(dest_addr));	//初始化
	dest_addr.sin_family = AF_INET;		//套接字域是AF_INET(网络套接字)

	pid = getpid();

	//当按下ctrl+c时发出中断信号，并开始执行统计函数
	signal(SIGINT,statistics);	
	while(nsend < MAX_NO_PACKETS){
		sleep(1);		//每隔一秒发送一个ICMP报文
		recv_packet();		//接收ICMP报文
	}
	return 0;
}



