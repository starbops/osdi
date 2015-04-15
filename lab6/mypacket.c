/*
 * mypacket.c
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/tcp.h>

#define DRIVER_AUTHOR "Zespre Schmidt <starbops@gmail.com>"
#define DRIVER_DESC "TCP payload encryption/decryptoin module"

extern void *mye;
extern void *myd;

int myencrypt(struct sk_buff *skb, int mykey)
{
	char to[128] = { 0 };
	int i = 0;
	if(mykey != 0) {
		skb_copy_bits(skb, 0, &to, skb->data_len);
		to[skb->data_len] = '\0';
		/* encrypt */
		printk(KERN_ALERT "Send side, the original message is : %s\n", to);
		for(i = 0; i < skb->data_len; i++)
			to[i] += mykey;
		printk(KERN_ALERT "Send side, the encrypted message is : %s\n", to);
		skb_store_bits(skb, 0, &to, skb->data_len);
	}
	return mykey;
}

int mydecrypt(struct sk_buff *skb, int mykey)
{
	struct tcphdr *th;
	char to[128] = { 0 };
	int i = 0;
	if(mykey != 0) {
		th = tcp_hdr(skb);
		skb_copy_bits(skb, th->doff*4, &to, skb->data_len);
		to[skb->data_len] = '\0';
		/* decrypt */
		printk(KERN_ALERT "Receive side, the encrypted message is : %s\n", to);
		for(i = 0; i < skb->data_len; i++)
			to[i] -= mykey;
		printk(KERN_ALERT "Receive side, the decrypted message is : %s\n", to);
		skb_store_bits(skb, th->doff*4, &to, skb->data_len);
	}
	return 0;
}

static int __init init_mypacket(void)
{
	printk(KERN_DEBUG "[mypacket] Up\n");
	mye = myencrypt;
	myd = mydecrypt;
	return 0;
}

static void __exit cleanup_mypacket(void)
{
	printk(KERN_DEBUG "[myPacket] Bye\n");
	mye = NULL;
	myd = NULL;
}

module_init(init_mypacket);
module_exit(cleanup_mypacket);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

