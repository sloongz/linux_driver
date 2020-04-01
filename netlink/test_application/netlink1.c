#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#define NETLINK_USER 23
#define MAX_MSGSIZE 1024

int main()
{
	struct sockaddr_nl nladdr;
	int skfd;
	struct msghdr msg;
	struct nlmsghdr *nlhdr;
	struct iovec iov;
	char buffer[] = "hello kernel!!!";

	/* 建立socket */
	skfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_USER);
	if(skfd == -1){
		perror("create socket error\n");
		return -1;
	}

	/* netlink socket */

	/*init netlink socket */
	nladdr.nl_family = AF_NETLINK;  /* AF_NETLINK or PE_NETLINK */
	nladdr.nl_pad = 0;              /* not use */
	nladdr.nl_pid = 0;              /* 传送到内核 */
	nladdr.nl_groups = 0;           /* 单播*/

	/* bing 绑定netlink socket 与socket */
	if( 0 != bind(skfd, (struct sockaddr *)&nladdr, sizeof(struct sockaddr_nl))){
		perror("bind() error\n");
		close(skfd);
		return -1;
	}

	/* 构造msg消息*/
	memset(&msg, 0, sizeof(msg));
	msg.msg_name = (void *)&(nladdr);
	msg.msg_namelen = sizeof(nladdr);


	/* netlink 消息头 */
	nlhdr = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_MSGSIZE));

	strcpy(NLMSG_DATA(nlhdr),buffer);

	nlhdr->nlmsg_len = NLMSG_LENGTH(strlen(buffer));
	nlhdr->nlmsg_pid = getpid();  /* self pid */
	nlhdr->nlmsg_flags = 0;

	iov.iov_base = (void *)nlhdr;
	iov.iov_len = nlhdr->nlmsg_len;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	sendmsg(skfd, &msg, 0);
	printf("pid:%d send msg\n", nlhdr->nlmsg_pid);

	/* recv */
	recvmsg(skfd, &msg, 0);
	printf("recv kernel msg: %s\n", (char *)NLMSG_DATA(nlhdr));
	
	close(skfd);
	free(nlhdr);

	return 0;
}

