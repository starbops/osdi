/*
 * This file is part of wl1271
 *
 * Copyright (C) 2009 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>

#include "wl1271.h"
#include "wl1271_spi.h"
#include "wl1271_io.h"
#include "wl1271_reg.h"
#include "wl1271_ps.h"
#include "wl1271_tx.h"

static int wl1271_tx_id(struct wl1271 *wl, struct sk_buff *skb)
{
	int i;
	for (i = 0; i < ACX_TX_DESCRIPTORS; i++)
		if (wl->tx_frames[i] == NULL) {
			wl->tx_frames[i] = skb;
			return i;
		}

	return -EBUSY;
}

static int wl1271_tx_allocate(struct wl1271 *wl, struct sk_buff *skb, u32 extra)
{
	struct wl1271_tx_hw_descr *desc;
	u32 total_len = skb->len + sizeof(struct wl1271_tx_hw_descr) + extra;
	u32 total_blocks, excluded;
	int id, ret = -EBUSY;

	/* allocate free identifier for the packet */
	id = wl1271_tx_id(wl, skb);
	if (id < 0)
		return id;

	/* approximate the number of blocks required for this packet
	   in the firmware */
	/* FIXME: try to figure out what is done here and make it cleaner */
	total_blocks = (total_len + 20) >> TX_HW_BLOCK_SHIFT_DIV;
	excluded = (total_blocks << 2) + ((total_len + 20) & 0xff) + 34;
	total_blocks += (excluded > 252) ? 2 : 1;
	total_blocks += TX_HW_BLOCK_SPARE;

	if (total_blocks <= wl->tx_blocks_available) {
		desc = (struct wl1271_tx_hw_descr *)skb_push(
			skb, total_len - skb->len);

		desc->extra_mem_blocks = TX_HW_BLOCK_SPARE;
		desc->total_mem_blocks = total_blocks;
		desc->id = id;

		wl->tx_blocks_available -= total_blocks;

		ret = 0;

		wl1271_debug(DEBUG_TX,
			     "tx_allocate: size: %d, blocks: %d, id: %d",
			     total_len, total_blocks, id);
	} else
		wl->tx_frames[id] = NULL;

	return ret;
}

static int wl1271_tx_fill_hdr(struct wl1271 *wl, struct sk_buff *skb,
			      u32 extra, struct ieee80211_tx_info *control)
{
	struct wl1271_tx_hw_descr *desc;
	int pad, ac;
	u16 tx_attr;

	desc = (struct wl1271_tx_hw_descr *) skb->data;

	/* relocate space for security header */
	if (extra) {
		void *framestart = skb->data + sizeof(*desc);
		u16 fc = *(u16 *)(framestart + extra);
		int hdrlen = ieee80211_hdrlen(cpu_to_le16(fc));
		memmove(framestart, framestart + extra, hdrlen);
	}

	/* configure packet life time */
	desc->start_time = cpu_to_le32(jiffies_to_usecs(jiffies) -
				       wl->time_offset);
	desc->life_time = cpu_to_le16(TX_HW_MGMT_PKT_LIFETIME_TU);

	/* configure the tx attributes */
	tx_attr = wl->session_counter << TX_HW_ATTR_OFST_SESSION_COUNTER;

	/* queue */
	ac = wl1271_tx_get_queue(skb_get_queue_mapping(skb));
	desc->tid = wl1271_tx_ac_to_tid(ac);

	desc->aid = TX_HW_DEFAULT_AID;
	desc->reserved = 0;

	/* align the length (and store in terms of words) */
	pad = WL1271_TX_ALIGN(skb->len);
	desc->length = cpu_to_le16(pad >> 2);

	/* calculate number of padding bytes */
	pad = pad - skb->len;
	tx_attr |= pad << TX_HW_ATTR_OFST_LAST_WORD_PAD;

	/* if the packets are destined for AP (have a STA entry) send them
	   with AP rate policies, otherwise use default basic rates */
	if (control->control.sta)
		tx_attr |= ACX_TX_AP_FULL_RATE << TX_HW_ATTR_OFST_RATE_POLICY;

	desc->tx_attr = cpu_to_le16(tx_attr);

	wl1271_debug(DEBUG_TX, "tx_fill_hdr: pad: %d", pad);
	return 0;
}

static int wl1271_tx_send_packet(struct wl1271 *wl, struct sk_buff *skb,
				 struct ieee80211_tx_info *control)
{

	struct wl1271_tx_hw_descr *desc;
	int len;

	/* FIXME: This is a workaround for getting non-aligned packets.
	   This happens at least with EAPOL packets from the user space.
	   Our DMA requires packets to be aligned on a 4-byte boundary.
	*/
	if (unlikely((long)skb->data & 0x03)) {
		int offset = (4 - (long)skb->data) & 0x03;
		wl1271_debug(DEBUG_TX, "skb offset %d", offset);

		/* check whether the current skb can be used */
		if (!skb_cloned(skb) && (skb_tailroom(skb) >= offset)) {
			unsigned char *src = skb->data;

			/* align the buffer on a 4-byte boundary */
			skb_reserve(skb, offset);
			memmove(skb->data, src, skb->len);
		} else {
			wl1271_info("No handler, fixme!");
			return -EINVAL;
		}
	}

	len = WL1271_TX_ALIGN(skb->len);

	/* perform a fixed address block write with the packet */
	wl1271_write(wl, WL1271_SLV_MEM_DATA, skb->data, len, true);

	/* write packet new counter into the write access register */
	wl->tx_packets_count++;
	wl1271_write32(wl, WL1271_HOST_WR_ACCESS, wl->tx_packets_count);

	desc = (struct wl1271_tx_hw_descr *) skb->data;
	wl1271_debug(DEBUG_TX, "tx id %u skb 0x%p payload %u (%u words)",
		     desc->id, skb, len, desc->length);

	return 0;
}

/* caller must hold wl->mutex */
static int wl1271_tx_frame(struct wl1271 *wl, struct sk_buff *skb)
{
	struct ieee80211_tx_info *info;
	u32 extra = 0;
	int ret = 0;
	u8 idx;

	if (!skb)
		return -EINVAL;

	info = IEEE80211_SKB_CB(skb);

	if (info->control.hw_key &&
	    info->control.hw_key->alg == ALG_TKIP)
		extra = WL1271_TKIP_IV_SPACE;

	if (info->control.hw_key) {
		idx = info->control.hw_key->hw_key_idx;

		/* FIXME: do we have to do this if we're not using WEP? */
		if (unlikely(wl->default_key != idx)) {
			ret = wl1271_cmd_set_default_wep_key(wl, idx);
			if (ret < 0)
				return ret;
			wl->default_key = idx;
		}
	}

	ret = wl1271_tx_allocate(wl, skb, extra);
	if (ret < 0)
		return ret;

	ret = wl1271_tx_fill_hdr(wl, skb, extra, info);
	if (ret < 0)
		return ret;

	ret = wl1271_tx_send_packet(wl, skb, info);
	if (ret < 0)
		return ret;

	return ret;
}

static u32 wl1271_tx_enabled_rates_get(struct wl1271 *wl, u32 rate_set)
{
	struct ieee80211_supported_band *band;
	u32 enabled_rates = 0;
	int bit;

	band = wl->hw->wiphy->bands[wl->band];
	for (bit = 0; bit < band->n_bitrates; bit++) {
		if (rate_set & 0x1)
			enabled_rates |= band->bitrates[bit].hw_value;
		rate_set >>= 1;
	}

	return enabled_rates;
}

void wl1271_tx_work(struct work_struct *work)
{
	struct wl1271 *wl = container_of(work, struct wl1271, tx_work);
	struct sk_buff *skb;
	bool woken_up = false;
	u32 sta_rates = 0;
	int ret;

	/* check if the rates supported by the AP have changed */
	if (unlikely(test_and_clear_bit(WL1271_FLAG_STA_RATES_CHANGED,
					&wl->flags))) {
		unsigned long flags;
		spin_lock_irqsave(&wl->wl_lock, flags);
		sta_rates = wl->sta_rate_set;
		spin_unlock_irqrestore(&wl->wl_lock, flags);
	}

	mutex_lock(&wl->mutex);

	if (unlikely(wl->state == WL1271_STATE_OFF))
		goto out;

	/* if rates have changed, re-configure the rate policy */
	if (unlikely(sta_rates)) {
		wl->rate_set = wl1271_tx_enabled_rates_get(wl, sta_rates);
		wl1271_acx_rate_policies(wl);
	}

	while ((skb = skb_dequeue(&wl->tx_queue))) {
		if (!woken_up) {
			ret = wl1271_ps_elp_wakeup(wl, false);
			if (ret < 0)
				goto out;
			woken_up = true;
		}

		ret = wl1271_tx_frame(wl, skb);
		if (ret == -EBUSY) {
			/* firmware buffer is full, stop queues */
			wl1271_debug(DEBUG_TX, "tx_work: fw buffer full, "
				     "stop queues");
			ieee80211_stop_queues(wl->hw);
			set_bit(WL1271_FLAG_TX_QUEUE_STOPPED, &wl->flags);
			skb_queue_head(&wl->tx_queue, skb);
			goto out;
		} else if (ret < 0) {
			dev_kfree_skb(skb);
			goto out;
		} else if (test_and_clear_bit(WL1271_FLAG_TX_QUEUE_STOPPED,
					      &wl->flags)) {
			/* firmware buffer has space, restart queues */
			wl1271_debug(DEBUG_TX,
				     "complete_packet: waking queues");
			ieee80211_wake_queues(wl->hw);
		}
	}

out:
	if (woken_up)
		wl1271_ps_elp_sleep(wl);

	mutex_unlock(&wl->mutex);
}

static void wl1271_tx_complete_packet(struct wl1271 *wl,
				      struct wl1271_tx_hw_res_descr *result)
{
	struct ieee80211_tx_info *info;
	struct sk_buff *skb;
	u16 seq;
	int id = result->id;

	/* check for id legality */
	if (id >= ACX_TX_DESCRIPTORS || wl->tx_frames[id] == NULL) {
		wl1271_warning("TX result illegal id: %d", id);
		return;
	}

	skb = wl->tx_frames[id];
	info = IEEE80211_SKB_CB(skb);

	/* update packet status */
	if (!(info->flags & IEEE80211_TX_CTL_NO_ACK)) {
		if (result->status == TX_SUCCESS)
			info->flags |= IEEE80211_TX_STAT_ACK;
		if (result->status & TX_RETRY_EXCEEDED) {
			/* FIXME */
			/* info->status.excessive_retries = 1; */
			wl->stats.excessive_retries++;
		}
	}

	/* FIXME */
	/* info->status.retry_count = result->ack_failures; */
	wl->stats.retry_count += result->ack_failures;

	/* update security sequence number */
	seq = wl->tx_security_seq_16 +
		(result->lsb_security_sequence_number -
		 wl->tx_security_last_seq);
	wl->tx_security_last_seq = result->lsb_security_sequence_number;

	if (seq < wl->tx_security_seq_16)
		wl->tx_security_seq_32++;
	wl->tx_security_seq_16 = seq;

	/* remove private header from packet */
	skb_pull(skb, sizeof(struct wl1271_tx_hw_descr));

	/* remove TKIP header space if present */
	if (info->control.hw_key &&
	    info->control.hw_key->alg == ALG_TKIP) {
		int hdrlen = ieee80211_get_hdrlen_from_skb(skb);
		memmove(skb->data + WL1271_TKIP_IV_SPACE, skb->data, hdrlen);
		skb_pull(skb, WL1271_TKIP_IV_SPACE);
	}

	wl1271_debug(DEBUG_TX, "tx status id %u skb 0x%p failures %u rate 0x%x"
		     " status 0x%x",
		     result->id, skb, result->ack_failures,
		     result->rate_class_index, result->status);

	/* return the packet to the stack */
	ieee80211_tx_status(wl->hw, skb);
	wl->tx_frames[result->id] = NULL;
}

/* Called upon reception of a TX complete interrupt */
void wl1271_tx_complete(struct wl1271 *wl, u32 count)
{
	struct wl1271_acx_mem_map *memmap =
		(struct wl1271_acx_mem_map *)wl->target_mem_map;
	u32 i;

	wl1271_debug(DEBUG_TX, "tx_complete received, packets: %d", count);

	/* read the tx results from the chipset */
	wl1271_read(wl, le32_to_cpu(memmap->tx_result),
		    wl->tx_res_if, sizeof(*wl->tx_res_if), false);

	/* verify that the result buffer is not getting overrun */
	if (count > TX_HW_RESULT_QUEUE_LEN) {
		wl1271_warning("TX result overflow from chipset: %d", count);
		count = TX_HW_RESULT_QUEUE_LEN;
	}

	/* process the results */
	for (i = 0; i < count; i++) {
		struct wl1271_tx_hw_res_descr *result;
		u8 offset = wl->tx_results_count & TX_HW_RESULT_QUEUE_LEN_MASK;

		/* process the packet */
		result =  &(wl->tx_res_if->tx_results_queue[offset]);
		wl1271_tx_complete_packet(wl, result);

		wl->tx_results_count++;
	}

	/* write host counter to chipset (to ack) */
	wl1271_write32(wl, le32_to_cpu(memmap->tx_result) +
		       offsetof(struct wl1271_tx_hw_res_if,
		       tx_result_host_counter),
		       le32_to_cpu(wl->tx_res_if->tx_result_fw_counter));
}

/* caller must hold wl->mutex */
void wl1271_tx_flush(struct wl1271 *wl)
{
	int i;
	struct sk_buff *skb;
	struct ieee80211_tx_info *info;

	/* TX failure */
/* 	control->flags = 0; FIXME */

	while ((skb = skb_dequeue(&wl->tx_queue))) {
		info = IEEE80211_SKB_CB(skb);

		wl1271_debug(DEBUG_TX, "flushing skb 0x%p", skb);

		if (!(info->flags & IEEE80211_TX_CTL_REQ_TX_STATUS))
				continue;

		ieee80211_tx_status(wl->hw, skb);
	}

	for (i = 0; i < ACX_TX_DESCRIPTORS; i++)
		if (wl->tx_frames[i] != NULL) {
			skb = wl->tx_frames[i];
			info = IEEE80211_SKB_CB(skb);

			if (!(info->flags & IEEE80211_TX_CTL_REQ_TX_STATUS))
				continue;

			ieee80211_tx_status(wl->hw, skb);
			wl->tx_frames[i] = NULL;
		}
}
