/*
 * a netlink kernel module: netlink
 *
 * Copyright (C) 2019 sloongz
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/types.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <net/net_namespace.h>

#define NETLINK_USER 23
struct sock *nl_sk = NULL;

//向用户空间发送消息的接口
int sendnlmsg(char *message,int pid)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;

	int slen = 0;

	if(!message || !nl_sk){
		return -1;
	}

	slen = strlen(message);

	// 为新的 sk_buffer申请空间
	skb = nlmsg_new(slen, GFP_ATOMIC);
	if(!skb){
		printk(KERN_ERR "my_net_link: alloc_skb Error./n");
		return -2;
	}

	//用nlmsg_put()来设置netlink消息头部
	nlh = nlmsg_put(skb, 0, 0, NETLINK_USER, slen, 0);
	if(nlh == NULL){
		printk("nlmsg_put failauer \n");
		nlmsg_free(skb);
		return -1;
	}

	memcpy(nlmsg_data(nlh), message, slen);

	//通过netlink_unicast()将消息发送用户空间由pid所指定了进程号的进程
	netlink_unicast(nl_sk, skb, pid, MSG_DONTWAIT);
	printk("send OK!\n");

	return 0;
}

static void nl_data_ready(struct sk_buff *skb)
{
	struct nlmsghdr *nlh = NULL;
	char *umsg = NULL;
	char kmsg[] = "hello users!!!";

	printk("%s\n", __func__);

	if(skb->len >= nlmsg_total_size(0))
	{   
		nlh = nlmsg_hdr(skb);
		umsg = NLMSG_DATA(nlh);
		if(umsg)
		{   
			printk("kernel recv from pid %d user: %s\n", nlh->nlmsg_pid, umsg);
			sendnlmsg (kmsg, nlh->nlmsg_pid);
		}   
	}   
}

struct netlink_kernel_cfg cfg = {
	.input = nl_data_ready, /* set recv callback */
};

//模块初始化
static int __init x_init(void)
{
	printk(KERN_INFO "netlink create\n");
	nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
	return 0;
}

//模块注销
static void __exit x_exit(void)
{
	printk(KERN_INFO "netlink release\n");
}

module_init(x_init);
module_exit(x_exit);

MODULE_AUTHOR("sloongz <sloongz@163.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("a netlink module");
MODULE_ALIAS("netlink module");
