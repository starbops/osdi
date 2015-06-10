/*
 * elevator noop
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/init.h>

struct noop_data {
	struct list_head queue;
};

static unsigned long last_rq_pos = 0;

static unsigned long diff_abs(unsigned long a, unsigned long b)
{
	if(a > b)
		return a - b;
	else
		return b - a;
}

static void noop_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}

static int noop_dispatch(struct request_queue *q, int force)
{
	struct noop_data *nd = q->elevator->elevator_data;
	unsigned long diff = -1, tmp_diff = 0;

	if (!list_empty(&nd->queue)) {
		struct request *rq, *tmp_rq;
		/* original select algorithm (FIFO) */
		rq = list_entry(nd->queue.next, struct request, queuelist);
		/* select the nearest (SSTF) */
		list_for_each_entry(tmp_rq, &nd->queue, queuelist) {
			tmp_diff = diff_abs(last_rq_pos, blk_rq_pos(tmp_rq));
			if(tmp_diff < diff) {
				diff = tmp_diff;
				rq = tmp_rq;
			}
		}
		list_del_init(&rq->queuelist);
		elv_dispatch_add_tail(q, rq);
		/* update last_rq_pos */
		last_rq_pos = blk_rq_pos(rq);
		printk("dispatch %llu\n", blk_rq_pos(rq));
		return 1;
	}
	return 0;
}

static void noop_add_request(struct request_queue *q, struct request *rq)
{
	struct noop_data *nd = q->elevator->elevator_data;

	list_add_tail(&rq->queuelist, &nd->queue);
	printk("add %llu\n", blk_rq_pos(rq));
}

static int noop_queue_empty(struct request_queue *q)
{
	struct noop_data *nd = q->elevator->elevator_data;

	return list_empty(&nd->queue);
}

static struct request *
noop_former_request(struct request_queue *q, struct request *rq)
{
	struct noop_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.prev == &nd->queue)
		return NULL;
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
noop_latter_request(struct request_queue *q, struct request *rq)
{
	struct noop_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.next == &nd->queue)
		return NULL;
	return list_entry(rq->queuelist.next, struct request, queuelist);
}

static void *noop_init_queue(struct request_queue *q)
{
	struct noop_data *nd;

	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if (!nd)
		return NULL;
	INIT_LIST_HEAD(&nd->queue);
	return nd;
}

static void noop_exit_queue(struct elevator_queue *e)
{
	struct noop_data *nd = e->elevator_data;

	BUG_ON(!list_empty(&nd->queue));
	kfree(nd);
}

static struct elevator_type elevator_noop = {
	.ops = {
		.elevator_merge_req_fn		= noop_merged_requests,
		.elevator_dispatch_fn		= noop_dispatch,
		.elevator_add_req_fn		= noop_add_request,
		.elevator_queue_empty_fn	= noop_queue_empty,
		.elevator_former_req_fn		= noop_former_request,
		.elevator_latter_req_fn		= noop_latter_request,
		.elevator_init_fn		= noop_init_queue,
		.elevator_exit_fn		= noop_exit_queue,
	},
	.elevator_name = "noop",
	.elevator_owner = THIS_MODULE,
};

static int __init noop_init(void)
{
	elv_register(&elevator_noop);

	return 0;
}

static void __exit noop_exit(void)
{
	elv_unregister(&elevator_noop);
}

module_init(noop_init);
module_exit(noop_exit);


MODULE_AUTHOR("Jens Axboe");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("No-op IO scheduler");
