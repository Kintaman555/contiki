/*
 * Copyright (c) 2015, SICS Swedish ICT.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         TSCH slot operation implementation, running from interrupt.
 * \author
 *         Simon Duquennoy <simonduq@sics.se>
 *         Beshr Al Nahas <beshr@sics.se>
 *
 */

#include "contiki.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/queuebuf.h"
#include "net/mac/framer-802154.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/tsch-slot-operation.h"
#include "net/mac/tsch/tsch-queue.h"
#include "net/mac/tsch/tsch-private.h"
#include "net/mac/tsch/tsch-log.h"
#include "net/mac/tsch/tsch-packet.h"
#include "net/mac/tsch/tsch-security.h"
#include "net/mac/tsch/tsch-adaptive-timesync.h"

#if TSCH_LOG_LEVEL >= 1
#define DEBUG DEBUG_PRINT
#else /* TSCH_LOG_LEVEL */
#define DEBUG DEBUG_NONE
#endif /* TSCH_LOG_LEVEL */
#include "net/ip/uip-debug.h"

/* For AL-MMAC integration, might be redundant */
#include "lib/memb.h"
#include "net/rime/collect-neighbor.h"
#include "net/rime/timesynch.h"
#include "net/rpl/rpl.h"
#include "lib/random.h"
#include "sys/rtimer.h"
#include <stdio.h>

/* TSCH debug macros, i.e. to set LEDs or GPIOs on various TSCH
 * timeslot events */
#ifndef TSCH_DEBUG_INIT
#define TSCH_DEBUG_INIT()
#endif
#ifndef TSCH_DEBUG_INTERRUPT
#define TSCH_DEBUG_INTERRUPT()
#endif
#ifndef TSCH_DEBUG_RX_EVENT
#define TSCH_DEBUG_RX_EVENT()
#endif
#ifndef TSCH_DEBUG_TX_EVENT
#define TSCH_DEBUG_TX_EVENT()
#endif
#ifndef TSCH_DEBUG_SLOT_START
#define TSCH_DEBUG_SLOT_START()
#endif
#ifndef TSCH_DEBUG_SLOT_END
#define TSCH_DEBUG_SLOT_END()
#endif

/* Check if TSCH_MAX_INCOMING_PACKETS is power of two */
#if (TSCH_MAX_INCOMING_PACKETS & (TSCH_MAX_INCOMING_PACKETS - 1)) != 0
#error TSCH_MAX_INCOMING_PACKETS must be power of two
#endif

/* Check if TSCH_DEQUEUED_ARRAY_SIZE is power of two and greater or equal to QUEUEBUF_NUM */
#if TSCH_DEQUEUED_ARRAY_SIZE < QUEUEBUF_NUM
#error TSCH_DEQUEUED_ARRAY_SIZE must be greater or equal to QUEUEBUF_NUM
#endif
#if (TSCH_DEQUEUED_ARRAY_SIZE & (TSCH_DEQUEUED_ARRAY_SIZE - 1)) != 0
#error TSCH_DEQUEUED_ARRAY_SIZE must be power of two
#endif

/* Truncate received drift correction information to maximum half
 * of the guard time (one fourth of TSCH_DEFAULT_TS_RX_WAIT) */
#define SYNC_IE_BOUND ((int32_t)US_TO_RTIMERTICKS(TSCH_DEFAULT_TS_RX_WAIT / 4))

/* By default: check that rtimer runs at >=32kHz and use a guard time of 10us */
#if RTIMER_SECOND < (32 * 1024)
#error "TSCH: RTIMER_SECOND < (32 * 1024)"
#endif
#if RTIMER_SECOND >= 200000
#define RTIMER_GUARD (RTIMER_SECOND / 100000)
#else
#define RTIMER_GUARD 2u
#endif

//#define n_ts		((timesynch_rtimer_to_time(RTIMER_NOW()) / TSCH_DEFAULT_TS_TIMESLOT_LENGTH) % (TSCH_SCHEDULE_DEFAULT_LENGTH + 1))

/*---------------======== for learning process ==========-----------------*/
#define FRAMES_START_EXPLOITATION (30 * TSCH_SCHEDULE_DEFAULT_LENGTH)
#define RANDOM_CHOOSE_PARENT 0    /* enable/disable choosing a new parent randomly when exploitation*/
#define PROBABILITY_SCALE		  100
#define EXPLOITATION_PROBABILITY  70 /*if prob of successful trial is smaller than EXPLOITATION_PROBABILITY,
										node randomly chooses a new parent, only used if disable RANDOM_CHOOSE_PARENT*/
#define SUCCESS_PROBABILITY		  30
#define MAX_CHECK_CCA		 8
#define WIN_THRESHOLD        3
#define LOSE_THRESHOLD		 5

#define BC_ALL_SLOTS_BEFORE_LEARNING 0 /* enable/disable all broadcast slots in neighbor discovery phase*/

const linkaddr_t sleep_addr = {{255, 255}};

/* A ringbuf storing outgoing packets after they were dequeued.
 * Will be processed layer by tsch_tx_process_pending */
struct ringbufindex dequeued_ringbuf;
struct tsch_packet *dequeued_array[TSCH_DEQUEUED_ARRAY_SIZE];
/* A ringbuf storing incoming packets.
 * Will be processed layer by tsch_rx_process_pending */
struct ringbufindex input_ringbuf;
struct input_packet input_array[TSCH_MAX_INCOMING_PACKETS];

struct learning_distribution {
	u8_t successful_slot;
	u8_t total_slot;
	u16_t successful_prob;
	int num; // num of check cca when contention, depends on # successful trials
};

struct learning_distributions {
	struct learning_distributions *next;
	struct learning_distribution distributions[TSCH_SCHEDULE_DEFAULT_LENGTH];
	linkaddr_t parent;
};

MEMB(learning_distribution_memb, struct learning_distributions, TSCH_SCHEDULE_MAX_SLOTFRAMES);
LIST(distribution_list);

struct parent {
	struct parent *next;
	linkaddr_t addr;
};
MEMB(parent_memb, struct rpl_parent, TSCH_SCHEDULE_MAX_SLOTFRAMES);
LIST(parent_list);

static struct collect_neighbor_list neighbor_list;
static u16_t my_rtmetric;

/* Learning status */
enum {
	LISTEN_MODE,
	TRANSMIT_MODE,
	SLEEP_MODE,
	NOT_YET_LEARNING,
	STILL_LEARNING,
	LEARNING_DONE
};

volatile rtimer_clock_t t0;

static linkaddr_t strategy[TSCH_SCHEDULE_DEFAULT_LENGTH];

/* Last time we received Sync-IE (ACK or data packet from a time source) */
static struct asn_t last_sync_asn;

/* A global lock for manipulating data structures safely from outside of interrupt */
static volatile int tsch_locked = 0;
/* As long as this is set, skip all slot operation */
static volatile int tsch_lock_requested = 0;

/* Last estimated drift in RTIMER ticks
 * (Sky: 1 tick = 30.517578125 usec exactly) */
static int32_t drift_correction = 0;
/* Is drift correction used? (Can be true even if drift_correction == 0) */
static uint8_t is_drift_correction_used;

/* The neighbor last used as our time source */
struct tsch_neighbor *last_timesource_neighbor = NULL;

/* Used from tsch_slot_operation and sub-protothreads */
static rtimer_clock_t volatile current_slot_start;

/* Are we currently inside a slot? */
static volatile int tsch_in_slot_operation = 0;

/* Info about the link, packet and neighbor of
 * the current (or next) slot */
struct tsch_link *current_link = NULL;
/* A backup link with Rx flag, overlapping with current_link.
 * If the current link is Tx-only and the Tx queue
 * is empty while executing the link, fallback to the backup link. */
static struct tsch_link *backup_link = NULL;
static struct tsch_packet *current_packet = NULL;
static struct tsch_neighbor *current_neighbor = NULL;

/* Protothread for association */
PT_THREAD(tsch_scan(struct pt *pt));
/* Protothread for slot operation, called from rtimer interrupt
 * and scheduled from tsch_schedule_slot_operation */
static PT_THREAD(tsch_slot_operation(struct rtimer *t, void *ptr));
static struct pt slot_operation_pt;
/* Sub-protothreads of tsch_slot_operation */
static PT_THREAD(tsch_tx_slot(struct pt *pt, struct rtimer *t));
static PT_THREAD(tsch_rx_slot(struct pt *pt, struct rtimer *t));

/*---------------------------------------------------------------------------*/

static volatile unsigned char radio_is_on;
static volatile unsigned char mode;
static volatile unsigned char we_are_learning;
static volatile unsigned char listen_for_message = 0;
static unsigned long num_learning_done = 0;

/*---------------------------------------------------------------------------*/

void update_from_net(struct collect_neighbor_list *list, u16_t rtmetric)
{
	if(we_are_learning != STILL_LEARNING){
	//we assume that our neighbor list and rtmetric are not changed during learning
	  neighbor_list = *list;
	  my_rtmetric = rtmetric;
	}
}

/*---------------------------------------------------------------------------*/

static void learning_init(void)
{
	memb_init(&learning_distribution_memb);
	list_init(distribution_list);
	memb_init(&parent_memb);
	list_init(parent_list);
	
	int i;
	struct collect_neighbor *n;
	struct learning_distributions *ld;
	
	/* Create a list of candidate parent from neighbor list, 
	 * initialize distribution list */
	for(n = list_head(neighbor_list.list); n != NULL; n = list_item_next(n)) {
		//printf("neighbor %d, n->rtmetric %d ", n->addr.u8[0],n->rtmetric);
		if((n->rtmetric + 1) == my_rtmetric){
			struct parent *p = memb_alloc(&parent_memb);
			ld = memb_alloc(&learning_distribution_memb);
			linkaddr_copy(&p->addr, &n->addr);
			linkaddr_copy(&ld->parent, &n->addr);
			for(i = 0; i < TSCH_SCHEDULE_DEFAULT_LENGTH; i++){
				ld->distributions[i].num = MAX_CHECK_CCA >> 1;
				ld->distributions[i].successful_slot = 0;
				ld->distributions[i].total_slot = 0;
			}
			list_add(parent_list, p);
			list_add(distribution_list, ld);
			//printf("add to parent list %d", p->addr.u8[0]);
		}
	}
	
	/* Listen trail */
	ld = memb_alloc(&learning_distribution_memb);
	linkaddr_copy(&ld->parent, &linkaddr_null);
	for(i = 0; i < TSCH_SCHEDULE_DEFAULT_LENGTH; i++) {
		ld->distributions[i].num = 0;
		ld->distributions[i].successful_slot = 0;
		ld->distributions[i].total_slot = 0;
	}
	list_add(distribution_list, ld);
	
	/* Randomly initialize strategy */
	for(i = 0; i < TSCH_SCHEDULE_DEFAULT_LENGTH; i++) {
		int index = random_rand() % (list_length(parent_list) + 1);
		if(index == list_length(parent_list)) {
			linkaddr_copy(&strategy[i], &linkaddr_null);
		} else {
			int j = 0;
			struct parent *p = NULL;
			for(p = list_head(parent_list); p != NULL; p = list_item_next(p)) {
				if (j == index) {
					linkaddr_copy(&strategy[i], &p->addr);
					break;
				}
				j++;
			}
		}
	}
}

/*---------------------------------------------------------------------------*/

#define LEARNING_DISABLE_BEACON 0
static int learning_disable_beacon(void){
#if LEARNING_DISABLE_BEACON
	return (we_are_learning == STILL_LEARNING);
#else
	return 0;
#endif
}

/*---------------------------------------------------------------------------*/

int learning_done(void){
	return (we_are_learning == LEARNING_DONE);
}

/*---------------------------------------------------------------------------*/

static void print_distribution_table(void)
{
  int i;
  for(i = 0; i < TSCH_SCHEDULE_DEFAULT_LENGTH; i ++){
	struct learning_distributions *ld;
	for(ld = list_head(distribution_list); ld != NULL; ld = list_item_next(ld)) {
		printf("slot %d, parent %d, total %d, success %d\n",
				i, ld->parent.u8[0],
				ld->distributions[i].total_slot,
				ld->distributions[i].successful_slot);

	}
  }
}

/*---------------------------------------------------------------------------*/
static void update_distribution_table(u8_t slot, linkaddr_t *parent, u8_t successful) {
	struct learning_distributions *ld;
	for(ld = list_head(distribution_list); ld != NULL; ld = list_item_next(ld)) {
		if(linkaddr_cmp(&ld->parent, parent)) {
			ld->distributions[slot].total_slot ++;
			if(successful) {
				ld->distributions[slot].successful_slot ++;
			}
			if(!linkaddr_cmp(parent, &linkaddr_null)) {
				u16_t fail, total;
				total = ld->distributions[slot].total_slot;
				fail = total - ld->distributions[slot].successful_slot;
				ld->distributions[slot].num = (MAX_CHECK_CCA * (fail + 1)) / (total + 2);
			}
			return;
		}
	}
	return;
}

/*---------------------------------------------------------------------------*/

static int check_cca_num(u8_t slot, linkaddr_t *parent) {
	if(we_are_learning == STILL_LEARNING) {
		struct learning_distributions *ld;
		for (ld = list_head(distribution_list); ld != NULL; ld = list_item_next(ld)) {
			if(linkaddr_cmp(&ld->parent, parent)) {
				if(ld->distributions[slot].num < WIN_THRESHOLD) {
					return 0;
				}
				if(ld->distributions[slot].num > LOSE_THRESHOLD) {
					return MAX_CHECK_CCA - 1;
				}
				return ld->distributions[slot].num;
			}
		}
	}
	return 2;
}

/*---------------------------------------------------------------------------*/

static void learning_fix_result(void) {
	int i;
	struct learning_distributions *ld;
	struct parent *p;
	for(i = 0; i < TSCH_SCHEDULE_DEFAULT_LENGTH; i++) {
		/* Larger than total_slot */
		u16_t total, success, prob, max_prob;
		total = 0, max_prob = 0;
		struct learning_distributions *best_ld;
		best_ld = NULL;
		
		/* Number of trials of slot */
		for(ld = list_head(distribution_list); ld != NULL; ld = list_item_next(ld)) {
			total += ld->distributions[i].total_slot;
		}
		
		/* Find the parent which has the most successful prob */
		for(ld = list_head(distribution_list); ld != NULL; ld = list_item_next(ld)) {
			success = ld->distributions[i].successful_slot;
			prob = (total) ? ((success * PROBABILITY_SCALE) / total) : 0;
			if(prob > max_prob) {
				max_prob = prob;
				best_ld = ld;
			}
		}
		
		/* Setting strategy */
		/* All probs = 0 or no item in distribution list */
		if(best_ld == NULL) {
			linkaddr_copy(&strategy[i], &sleep_addr);
		} else {
			if(max_prob >= SUCCESS_PROBABILITY) {
				linkaddr_copy(&strategy[i], &best_ld->parent);
			} else {
				linkaddr_copy(&strategy[i], &sleep_addr);
			}
			printf("slot\t%d\ts\t%d\tp\t%d\ttotal\t%d\tset\t%d\n", i+1,
                			best_ld->parent.u8[0], max_prob, total, strategy[i].u8[0]);
		}
	}
	
	/* Free learning_distribution_list member */
	for(ld = list_head(distribution_list); ld != NULL; ld = list_item_next(ld)) {
		memb_free(&learning_distribution_memb, ld);
	}
	
	/* Free parent_list member */
	for(p = list_head(parent_list); p != NULL; p = list_item_next(p)) {
		memb_free(&parent_memb, p);
	}
	
	printf("DONE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
}

/*---------------------------------------------------------------------------*/

static void exploitation(u8_t slot) {
#if RANDOM_CHOOSE_PARENT
	int index = random_rand() % (list_length(parent_list + 1);
	if(index == list_length(parent_list)) {
		linkaddr_copy(&strategy[slot], &linkaddr_null);
	} else {
		int j = 0;
		struct parent *p = NULL;
		for (p = list_head(parent_list); p != NULL; p = list_item_next(p)) {
			if(j == index) {
				linkaddr_copy(&strategy[slot], &p->addr);
				break;
			}
			j++;
		}
	}
#else
	/* Randomly choose parent in the first 30 frames */
	if(num_learning_done < FRAMES_START_EXPLOITATION) {
		int index = random_rand() % (list_length(parent_list) + 1);
		if(index == list_length(parent_list)) {
			linkaddr_copy(&strategy[slot], &linkaddr_null);
		} else {
			int j = 0;
			struct parent *p = NULL;
			for(p = list_head(parent_list); p != NULL; p = list_item_next(p)) {
				if(j == index) {
					linkaddr_copy(&strategy[slot], &p->addr);
				}
				j++;
			}
		}
		return;
	}
	
	struct learning_distributions *ld, *best_ld = NULL;
	u16_t total = 0, prob, max_prob = 0;
	
	for(ld = list_head(distribution_list); ld != NULL; ld = list_item_next(ld)) {
		prob = ((2 + ld->distributions[slot].successful_slot) * PROBABILITY_SCALE) / (ld->distributions[slot].total_slot + 4);
		if(prob > max_prob) {
			max_prob = prob;
			best_ld = ld;
		}
	}
	
	if(best_ld == NULL || (max_prob < EXPLOITATION_PROBABILITY)) {
		int index = random_rand() % (list_length(parent_list) + 1);
		if(index == list_length(parent_list)) {
			linkaddr_copy(&strategy[slot], &linkaddr_null);
		} else {
			int j = 0;
			struct parent *p = NULL;
			for(p = list_head(parent_list); p != NULL; p = list_item_next(p)) {
				if(j == index) {
					linkaddr_copy(&strategy[slot], &p->addr);
					break;
				}
				j++;
			}
		}
	} else {
		linkaddr_copy(&strategy[slot], &best_ld->parent);
	}
#endif /* RANDOM_CHOOSE_PARENT */
}

/*---------------------------------------------------------------------------*/

static struct ctimer cycle_check_timer;
#define CHECK_LEARNING_PERIOD (CLOCK_SECOND * 2)
/* Number of frames learning, must be less than total_slot to avoid overflow
 * Currently ise u8_t (Max value = 255) */
#define MAX_LEARNING_SLOTS (TSCH_SCHEDULE_DEFAULT_LENGTH * 150)
static void cycle_check_learning(void) {
	if(num_learning_done >= MAX_LEARNING_SLOTS) {
		we_are_learning = LEARNING_DONE;
		learning_fix_result();
	}
	if(we_are_learning == STILL_LEARNING) {
		ctimer_set(&cycle_check_timer, CHECK_LEARNING_PERIOD, (void *) cycle_check_learning, NULL);
	}
}

/*---------------------------------------------------------------------------*/

void learning_start(void) {
	num_learning_done = 0;
	printf("START LEARNING!!!!!!!!!!!!!!!!!!!!!");
	learning_init();
	we_are_learning = STILL_LEARNING;
}

/*---------------------------------------------------------------------------*/
/* TSCH locking system. TSCH is locked during slot operations */

/* Is TSCH locked? */
int
tsch_is_locked(void)
{
  return tsch_locked;
}

/* Lock TSCH (no slot operation) */
int
tsch_get_lock(void)
{
  if(!tsch_locked) {
    rtimer_clock_t busy_wait_time;
    int busy_wait = 0; /* Flag used for logging purposes */
    /* Make sure no new slot operation will start */
    tsch_lock_requested = 1;
    /* Wait for the end of current slot operation. */
    if(tsch_in_slot_operation) {
      busy_wait = 1;
      busy_wait_time = RTIMER_NOW();
      while(tsch_in_slot_operation);
      busy_wait_time = RTIMER_NOW() - busy_wait_time;
    }
    if(!tsch_locked) {
      /* Take the lock if it is free */
      tsch_locked = 1;
      tsch_lock_requested = 0;
      if(busy_wait) {
        /* Issue a log whenever we had to busy wait until getting the lock */
        TSCH_LOG_ADD(tsch_log_message,
            snprintf(log->message, sizeof(log->message),
                "!get lock delay %u", (unsigned)busy_wait_time);
        );
      }
      return 1;
    }
  }
  TSCH_LOG_ADD(tsch_log_message,
      snprintf(log->message, sizeof(log->message),
                      "!failed to lock");
          );
  return 0;
}

/* Release TSCH lock */
void
tsch_release_lock(void)
{
  tsch_locked = 0;
}

/*---------------------------------------------------------------------------*/
/* Channel hopping utility functions */

/* Return channel from ASN and channel offset */
uint8_t
tsch_calculate_channel(struct asn_t *asn, uint8_t channel_offset)
{
  uint16_t index_of_0 = ASN_MOD(*asn, tsch_hopping_sequence_length);
  uint16_t index_of_offset = (index_of_0 + channel_offset) % tsch_hopping_sequence_length.val;
  return tsch_hopping_sequence[index_of_offset];
}

/*---------------------------------------------------------------------------*/
/* Timing utility functions */

/* Checks if the current time has passed a ref time + offset. Assumes
 * a single overflow and ref time prior to now. */
static uint8_t
check_timer_miss(rtimer_clock_t ref_time, rtimer_clock_t offset, rtimer_clock_t now)
{
  rtimer_clock_t target = ref_time + offset;
  int now_has_overflowed = now < ref_time;
  int target_has_overflowed = target < ref_time;

  if(now_has_overflowed == target_has_overflowed) {
    /* Both or none have overflowed, just compare now to the target */
    return target <= now;
  } else {
    /* Either now or target of overflowed.
     * If it is now, then it has passed the target.
     * If it is target, then we haven't reached it yet.
     *  */
    return now_has_overflowed;
  }
}
/*---------------------------------------------------------------------------*/
/* Schedule a wakeup at a specified offset from a reference time.
 * Provides basic protection against missed deadlines and timer overflows
 * A return value of zero signals a missed deadline: no rtimer was scheduled. */
static uint8_t
tsch_schedule_slot_operation(struct rtimer *tm, rtimer_clock_t ref_time, rtimer_clock_t offset, const char *str)
{
  rtimer_clock_t now = RTIMER_NOW();
  int r;
  /* Subtract RTIMER_GUARD before checking for deadline miss
   * because we can not schedule rtimer less than RTIMER_GUARD in the future */
  int missed = check_timer_miss(ref_time, offset - RTIMER_GUARD, now);

  if(missed) {
    TSCH_LOG_ADD(tsch_log_message,
                snprintf(log->message, sizeof(log->message),
                    "!dl-miss %s %d %d",
                        str, (int)(now-ref_time), (int)offset);
    );

    return 0;
  }
  ref_time += offset;
  r = rtimer_set(tm, ref_time, 1, (void (*)(struct rtimer *, void *))tsch_slot_operation, NULL);
  if(r != RTIMER_OK) {
    return 0;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
/* Schedule slot operation conditionally, and YIELD if success only.
 * Always attempt to schedule RTIMER_GUARD before the target to make sure to wake up
 * ahead of time and then busy wait to exactly hit the target. */
#define TSCH_SCHEDULE_AND_YIELD(pt, tm, ref_time, offset, str) \
  do { \
    if(tsch_schedule_slot_operation(tm, ref_time, offset - RTIMER_GUARD, str)) { \
      PT_YIELD(pt); \
    } \
    BUSYWAIT_UNTIL_ABS(0, ref_time, offset); \
  } while(0);
/*---------------------------------------------------------------------------*/
/* Get EB, broadcast or unicast packet to be sent, and target neighbor. */
static struct tsch_packet *
get_packet_and_neighbor_for_link(struct tsch_link *link, struct tsch_neighbor **target_neighbor, struct asn_t *asn)
{
  struct tsch_packet *p = NULL;
  struct tsch_neighbor *n = NULL;

/*--------------------------------This is where things get ugly-----------------------------------*/  
  
  /* Is this a Tx link? */
  int n_ts = (int)asn % TSCH_SCHEDULE_DEFAULT_LENGTH;
  if(link->link_options & LINK_OPTION_TX) {
    /* is it for advertisement of EB? */
    if(link->link_type == LINK_TYPE_ADVERTISING || link->link_type == LINK_TYPE_ADVERTISING_ONLY) {
      /* fetch EB packets */
      n = n_eb;
	  if(n_ts == 0) {
	  	p = tsch_queue_get_packet_for_nbr(n, link);
	  }
    }
    if(link->link_type != LINK_TYPE_ADVERTISING_ONLY) {
      /* NORMAL link or no EB to send, pick a data packet */
      if(p == NULL) {
        /* Get neighbor queue associated to the link and get packet from it */
        n = tsch_queue_get_nbr(&link->addr);
		if(n_ts == 0) {
			p = tsch_queue_get_packet_for_nbr(n, link);
		}
        /* if it is a broadcast slot and there were no broadcast packets, pick any unicast packet */
        if(p == NULL && n == n_broadcast) {
			if(n_ts != 0) {
				p = tsch_queue_get_unicast_packet_for_any(&n, link);
			}
        }
      }
    }
  }
  /* return nbr (by reference) */
  if(target_neighbor != NULL) {
    *target_neighbor = n;
  }

  return p;
}
/*---------------------------------------------------------------------------*/
/* Post TX: Update neighbor state after a transmission */
static int
update_neighbor_state(struct tsch_neighbor *n, struct tsch_packet *p,
                      struct tsch_link *link, uint8_t mac_tx_status, struct asn_t *asn)
{
  int in_queue = 1;
  int is_shared_link = link->link_options & LINK_OPTION_SHARED;
  int is_unicast = !n->is_broadcast;

  if(mac_tx_status == MAC_TX_OK) {
    /* Successful transmission */
    tsch_queue_remove_packet_from_queue(n);
    in_queue = 0;

    /* Update CSMA state in the unicast case */
    if(is_unicast) {
      if(is_shared_link || tsch_queue_is_empty(n)) {
        /* If this is a shared link, reset backoff on success.
         * Otherwise, do so only is the queue is empty */
        tsch_queue_backoff_reset(n);
      }
    }
  } else {
    /* Failed transmission */
    if(p->transmissions >= TSCH_MAC_MAX_FRAME_RETRIES + 1) {
      /* Drop packet */
      tsch_queue_remove_packet_from_queue(n);
      in_queue = 0;
    }
    /* Update CSMA state in the unicast case */
    if(is_unicast) {
      /* Failures on dedicated (== non-shared) leave the backoff
       * window nor exponent unchanged */
      if(is_shared_link) {
        /* Shared link: increment backoff exponent, pick a new window */
        tsch_queue_backoff_inc(n);
      }
	  
	  if(p->transmissions >= TSCH_MAC_MAX_FRAME_RETRIES + 1) {
      	if(we_are_learning == STILL_LEARNING){
		  
		  int n_ts = (int)asn % TSCH_SCHEDULE_DEFAULT_LENGTH;
      	  update_distribution_table(n_ts - 1, &strategy[n_ts - 1], 0);
      	  exploitation(n_ts - 1);
      	}
	  }
    }
  }

  return in_queue;
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(tsch_tx_slot(struct pt *pt, struct rtimer *t))
{
  /**
   * TX slot:
   * 1. Copy packet to radio buffer
   * 2. Perform CCA if enabled
   * 3. Sleep until it is time to transmit
   * 4. Wait for ACK if it is a unicast packet
   * 5. Extract drift if we received an E-ACK from a time source neighbor
   * 6. Update CSMA parameters according to TX status
   * 7. Schedule mac_call_sent_callback
   **/

  /* tx status */
  static uint8_t mac_tx_status;
  /* is the packet in its neighbor's queue? */
  uint8_t in_queue;
  static int dequeued_index;
  static int packet_ready = 1;

  PT_BEGIN(pt);

  TSCH_DEBUG_TX_EVENT();

  /* First check if we have space to store a newly dequeued packet (in case of
   * successful Tx or Drop) */
  dequeued_index = ringbufindex_peek_put(&dequeued_ringbuf);
  if(dequeued_index != -1) {
    if(current_packet == NULL || current_packet->qb == NULL) {
      mac_tx_status = MAC_TX_ERR_FATAL;
    } else {
      /* packet payload */
      static void *packet;
#if TSCH_SECURITY_ENABLED
      /* encrypted payload */
      static uint8_t encrypted_packet[TSCH_PACKET_MAX_LEN];
#endif /* TSCH_SECURITY_ENABLED */
      /* packet payload length */
      static uint8_t packet_len;
      /* packet seqno */
      static uint8_t seqno;
      /* is this a broadcast packet? (wait for ack?) */
      static uint8_t is_broadcast;
      static rtimer_clock_t tx_start_time;

#if CCA_ENABLED
      static uint8_t cca_status;
#endif

      /* get payload */
      packet = queuebuf_dataptr(current_packet->qb);
      packet_len = queuebuf_datalen(current_packet->qb);
      /* is this a broadcast packet? (wait for ack?) */
      is_broadcast = current_neighbor->is_broadcast;
      /* read seqno from payload */
      seqno = ((uint8_t *)(packet))[2];
      /* if this is an EB, then update its Sync-IE */
      if(current_neighbor == n_eb) {
        packet_ready = tsch_packet_update_eb(packet, packet_len, current_packet->tsch_sync_ie_offset);
      } else {
        packet_ready = 1;
      }

#if TSCH_SECURITY_ENABLED
      if(tsch_is_pan_secured) {
        /* If we are going to encrypt, we need to generate the output in a separate buffer and keep
         * the original untouched. This is to allow for future retransmissions. */
        int with_encryption = queuebuf_attr(current_packet->qb, PACKETBUF_ATTR_SECURITY_LEVEL) & 0x4;
        packet_len += tsch_security_secure_frame(packet, with_encryption ? encrypted_packet : packet, current_packet->header_len,
            packet_len - current_packet->header_len, &current_asn);
        if(with_encryption) {
          packet = encrypted_packet;
        }
      }
#endif /* TSCH_SECURITY_ENABLED */

      /* prepare packet to send: copy to radio buffer */
      if(packet_ready && NETSTACK_RADIO.prepare(packet, packet_len) == 0) { /* 0 means success */
        static rtimer_clock_t tx_duration;

#if CCA_ENABLED
        cca_status = 1;
        /* delay before CCA */
        TSCH_SCHEDULE_AND_YIELD(pt, t, current_slot_start, TS_CCA_OFFSET, "cca");
        TSCH_DEBUG_TX_EVENT();
        NETSTACK_RADIO.on();
        /* CCA */
        BUSYWAIT_UNTIL_ABS(!(cca_status |= NETSTACK_RADIO.channel_clear()),
                           current_slot_start, TS_CCA_OFFSET + TS_CCA);
        TSCH_DEBUG_TX_EVENT();
        /* there is not enough time to turn radio off */
        /*  NETSTACK_RADIO.off(); */
        if(cca_status == 0) {
          mac_tx_status = MAC_TX_COLLISION;
        } else
#endif /* CCA_ENABLED */
        {
          /* delay before TX */
          TSCH_SCHEDULE_AND_YIELD(pt, t, current_slot_start, tsch_timing[tsch_ts_tx_offset] - RADIO_DELAY_BEFORE_TX, "TxBeforeTx");
          TSCH_DEBUG_TX_EVENT();
          /* send packet already in radio tx buffer */
          mac_tx_status = NETSTACK_RADIO.transmit(packet_len);
          /* Save tx timestamp */
          tx_start_time = current_slot_start + tsch_timing[tsch_ts_tx_offset];
          /* calculate TX duration based on sent packet len */
          tx_duration = TSCH_PACKET_DURATION(packet_len);
          /* limit tx_time to its max value */
          tx_duration = MIN(tx_duration, tsch_timing[tsch_ts_max_tx]);
          /* turn tadio off -- will turn on again to wait for ACK if needed */
          NETSTACK_RADIO.off();

          if(mac_tx_status == RADIO_TX_OK) {
            if(!is_broadcast) {
              uint8_t ackbuf[TSCH_PACKET_MAX_LEN];
              int ack_len;
              rtimer_clock_t ack_start_time;
              int is_time_source;
              radio_value_t radio_rx_mode;
              struct ieee802154_ies ack_ies;
              uint8_t ack_hdrlen;
              frame802154_t frame;

              /* Entering promiscuous mode so that the radio accepts the enhanced ACK */
              NETSTACK_RADIO.get_value(RADIO_PARAM_RX_MODE, &radio_rx_mode);
              NETSTACK_RADIO.set_value(RADIO_PARAM_RX_MODE, radio_rx_mode & (~RADIO_RX_MODE_ADDRESS_FILTER));
              /* Unicast: wait for ack after tx: sleep until ack time */
              TSCH_SCHEDULE_AND_YIELD(pt, t, current_slot_start,
                  tsch_timing[tsch_ts_tx_offset] + tx_duration + tsch_timing[tsch_ts_rx_ack_delay] - RADIO_DELAY_BEFORE_RX, "TxBeforeAck");
              TSCH_DEBUG_TX_EVENT();
              NETSTACK_RADIO.on();
              /* Wait for ACK to come */
              BUSYWAIT_UNTIL_ABS(NETSTACK_RADIO.receiving_packet(),
                  tx_start_time, tx_duration + tsch_timing[tsch_ts_rx_ack_delay] + tsch_timing[tsch_ts_ack_wait]);
              TSCH_DEBUG_TX_EVENT();

              ack_start_time = RTIMER_NOW();

              /* Wait for ACK to finish */
              BUSYWAIT_UNTIL_ABS(!NETSTACK_RADIO.receiving_packet(),
                                 ack_start_time, tsch_timing[tsch_ts_max_ack]);
              TSCH_DEBUG_TX_EVENT();
              NETSTACK_RADIO.off();

              /* Leaving promiscuous mode */
              NETSTACK_RADIO.get_value(RADIO_PARAM_RX_MODE, &radio_rx_mode);
              NETSTACK_RADIO.set_value(RADIO_PARAM_RX_MODE, radio_rx_mode | RADIO_RX_MODE_ADDRESS_FILTER);

              /* Read ack frame */
              ack_len = NETSTACK_RADIO.read((void *)ackbuf, sizeof(ackbuf));

              is_time_source = 0;
              /* The radio driver should return 0 if no valid packets are in the rx buffer */
              if(ack_len > 0) {
				/* ACK received */
				
				if(we_are_learning == STILL_LEARNING){
					int n_ts = (int)&current_asn % TSCH_SCHEDULE_DEFAULT_LENGTH;
					update_distribution_table(n_ts - 1, &strategy[n_ts - 1], 1);
				}
				
                is_time_source = current_neighbor != NULL && current_neighbor->is_time_source;
                if(tsch_packet_parse_eack(ackbuf, ack_len, seqno,
                    &frame, &ack_ies, &ack_hdrlen) == 0) {
                  ack_len = 0;
                }

#if TSCH_SECURITY_ENABLED
                if(ack_len != 0) {
                  if(!tsch_security_parse_frame(ackbuf, ack_hdrlen, ack_len - ack_hdrlen - tsch_security_mic_len(&frame),
                      &frame, &current_neighbor->addr, &current_asn)) {
                    TSCH_LOG_ADD(tsch_log_message,
                        snprintf(log->message, sizeof(log->message),
                        "!failed to authenticate ACK"));
                    ack_len = 0;
                  }
                } else {
                  TSCH_LOG_ADD(tsch_log_message,
                      snprintf(log->message, sizeof(log->message),
                      "!failed to parse ACK"));
                }
#endif /* TSCH_SECURITY_ENABLED */
              } else {
				
	  			if(we_are_learning == STILL_LEARNING){
				  int n_ts = (int)&current_asn % TSCH_SCHEDULE_DEFAULT_LENGTH;
	  			  printf("slot %d: not got ACK, transmission failed to %d\n", n_ts, strategy[n_ts - 1].u8[0]);
	  			  update_distribution_table(n_ts - 1, &strategy[n_ts - 1], 0);
	  			  exploitation(n_ts - 1);
	  			}
              }

              if(ack_len != 0) {
                if(is_time_source) {
                  int32_t eack_time_correction = US_TO_RTIMERTICKS(ack_ies.ie_time_correction);
                  int32_t since_last_timesync = ASN_DIFF(current_asn, last_sync_asn);
                  if(eack_time_correction > SYNC_IE_BOUND) {
                    drift_correction = SYNC_IE_BOUND;
                  } else if(eack_time_correction < -SYNC_IE_BOUND) {
                    drift_correction = -SYNC_IE_BOUND;
                  } else {
                    drift_correction = eack_time_correction;
                  }
                  if(drift_correction != eack_time_correction) {
                    TSCH_LOG_ADD(tsch_log_message,
                        snprintf(log->message, sizeof(log->message),
                            "!truncated dr %d %d", (int)eack_time_correction, (int)drift_correction);
                    );
                  }
                  is_drift_correction_used = 1;
                  tsch_timesync_update(current_neighbor, since_last_timesync, drift_correction);
                  /* Keep track of sync time */
                  last_sync_asn = current_asn;
                  tsch_schedule_keepalive();
                }
                mac_tx_status = MAC_TX_OK;
              } else {
                mac_tx_status = MAC_TX_NOACK;
              }
            } else {
              mac_tx_status = MAC_TX_OK;
            }
          } else {
            mac_tx_status = MAC_TX_ERR;
          }
        }
      }
    }

    current_packet->transmissions++;
    current_packet->ret = mac_tx_status;

    /* Post TX: Update neighbor state */
    in_queue = update_neighbor_state(current_neighbor, current_packet, current_link, mac_tx_status, &current_asn);

    /* The packet was dequeued, add it to dequeued_ringbuf for later processing */
    if(in_queue == 0) {
      dequeued_array[dequeued_index] = current_packet;
      ringbufindex_put(&dequeued_ringbuf);
    }

    /* Log every tx attempt */
    TSCH_LOG_ADD(tsch_log_tx,
        log->tx.mac_tx_status = mac_tx_status;
    log->tx.num_tx = current_packet->transmissions;
    log->tx.datalen = queuebuf_datalen(current_packet->qb);
    log->tx.drift = drift_correction;
    log->tx.drift_used = is_drift_correction_used;
    log->tx.is_data = ((((uint8_t *)(queuebuf_dataptr(current_packet->qb)))[0]) & 7) == FRAME802154_DATAFRAME;
    log->tx.sec_level = queuebuf_attr(current_packet->qb, PACKETBUF_ATTR_SECURITY_LEVEL);
    log->tx.dest = TSCH_LOG_ID_FROM_LINKADDR(queuebuf_addr(current_packet->qb, PACKETBUF_ADDR_RECEIVER));
    );

    /* Poll process for later processing of packet sent events and logs */
    process_poll(&tsch_pending_events_process);
  }

  TSCH_DEBUG_TX_EVENT();

  PT_END(pt);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(tsch_rx_slot(struct pt *pt, struct rtimer *t))
{
  /**
   * RX slot:
   * 1. Check if it is used for TIME_KEEPING
   * 2. Sleep and wake up just before expected RX time (with a guard time: TS_LONG_GT)
   * 3. Check for radio activity for the guard time: TS_LONG_GT
   * 4. Prepare and send ACK if needed
   * 5. Drift calculated in the ACK callback registered with the radio driver. Use it if receiving from a time source neighbor.
   **/

  struct tsch_neighbor *n;
  static linkaddr_t source_address;
  static linkaddr_t destination_address;
  static int16_t input_index;
  static int input_queue_drop = 0;

  PT_BEGIN(pt);

  TSCH_DEBUG_RX_EVENT();

  input_index = ringbufindex_peek_put(&input_ringbuf);
  if(input_index == -1) {
    input_queue_drop++;
  } else {
    static struct input_packet *current_input;
    /* Estimated drift based on RX time */
    static int32_t estimated_drift;
    /* Rx timestamps */
    static rtimer_clock_t rx_start_time;
    static rtimer_clock_t expected_rx_time;
    static rtimer_clock_t packet_duration;
    uint8_t packet_seen;

    expected_rx_time = current_slot_start + tsch_timing[tsch_ts_tx_offset];
    /* Default start time: expected Rx time */
    rx_start_time = expected_rx_time;

    current_input = &input_array[input_index];

    /* Wait before starting to listen */
    TSCH_SCHEDULE_AND_YIELD(pt, t, current_slot_start, tsch_timing[tsch_ts_rx_offset] - RADIO_DELAY_BEFORE_RX, "RxBeforeListen");
    TSCH_DEBUG_RX_EVENT();

    /* Start radio for at least guard time */
    NETSTACK_RADIO.on();
    packet_seen = NETSTACK_RADIO.receiving_packet();
    if(!packet_seen) {
      /* Check if receiving within guard time */
      BUSYWAIT_UNTIL_ABS((packet_seen = NETSTACK_RADIO.receiving_packet()),
          current_slot_start, tsch_timing[tsch_ts_rx_offset] + tsch_timing[tsch_ts_rx_wait]);
    }
    if(packet_seen) {
      TSCH_DEBUG_RX_EVENT();
      /* Save packet timestamp */
      rx_start_time = RTIMER_NOW() - RADIO_DELAY_BEFORE_DETECT;
    }
    if(!NETSTACK_RADIO.receiving_packet() && !NETSTACK_RADIO.pending_packet()) {
      NETSTACK_RADIO.off();
      /* no packets on air */
	  
	  int n_ts = (int)&current_asn % TSCH_SCHEDULE_DEFAULT_LENGTH;
	  if(we_are_learning == STILL_LEARNING && n_ts) {
		printf("slot %d: no incoming packet, listen failed\n", n_ts);
		update_distribution_table(n_ts - 1, &linkaddr_null, 0);
		exploitation(n_ts - 1);
	  }
    } else {
      /* Wait until packet is received, turn radio off */
      BUSYWAIT_UNTIL_ABS(!NETSTACK_RADIO.receiving_packet(),
          current_slot_start, tsch_timing[tsch_ts_rx_offset] + tsch_timing[tsch_ts_rx_wait] + tsch_timing[tsch_ts_max_tx]);
      TSCH_DEBUG_RX_EVENT();
      NETSTACK_RADIO.off();

#if TSCH_RESYNC_WITH_SFD_TIMESTAMPS
      /* At the end of the reception, get an more accurate estimate of SFD arrival time */
      NETSTACK_RADIO.get_object(RADIO_PARAM_LAST_PACKET_TIMESTAMP, &rx_start_time, sizeof(rtimer_clock_t));
#endif

      if(NETSTACK_RADIO.pending_packet()) {
        static int frame_valid;
        static int header_len;
        static frame802154_t frame;
        radio_value_t radio_last_rssi;

        NETSTACK_RADIO.get_value(RADIO_PARAM_LAST_RSSI, &radio_last_rssi);
        /* Read packet */
        current_input->len = NETSTACK_RADIO.read((void *)current_input->payload, TSCH_PACKET_MAX_LEN);
        current_input->rx_asn = current_asn;
        current_input->rssi = (signed)radio_last_rssi;
        header_len = frame802154_parse((uint8_t *)current_input->payload, current_input->len, &frame);
        frame_valid = header_len > 0 &&
          frame802154_check_dest_panid(&frame) &&
          frame802154_extract_linkaddr(&frame, &source_address, &destination_address);

        packet_duration = TSCH_PACKET_DURATION(current_input->len);

#if TSCH_SECURITY_ENABLED
        /* Decrypt and verify incoming frame */
        if(frame_valid) {
          if(tsch_security_parse_frame(
               current_input->payload, header_len, current_input->len - header_len - tsch_security_mic_len(&frame),
               &frame, &source_address, &current_asn)) {
            current_input->len -= tsch_security_mic_len(&frame);
          } else {
            TSCH_LOG_ADD(tsch_log_message,
                snprintf(log->message, sizeof(log->message),
                "!failed to authenticate frame %u", current_input->len));
            frame_valid = 0;
          }
        } else {
          TSCH_LOG_ADD(tsch_log_message,
              snprintf(log->message, sizeof(log->message),
              "!failed to parse frame %u %u", header_len, current_input->len));
          frame_valid = 0;
        }
#endif /* TSCH_SECURITY_ENABLED */

        if(frame_valid) {
          if(linkaddr_cmp(&destination_address, &linkaddr_node_addr)
             || linkaddr_cmp(&destination_address, &linkaddr_null)) {
            int do_nack = 0;
            estimated_drift = ((int32_t)expected_rx_time - (int32_t)rx_start_time);

#if TSCH_TIMESYNC_REMOVE_JITTER
            /* remove jitter due to measurement errors */
            if(abs(estimated_drift) <= TSCH_TIMESYNC_MEASUREMENT_ERROR) {
              estimated_drift = 0;
            } else if(estimated_drift > 0) {
              estimated_drift -= TSCH_TIMESYNC_MEASUREMENT_ERROR;
            } else {
              estimated_drift += TSCH_TIMESYNC_MEASUREMENT_ERROR;
            }
#endif

#ifdef TSCH_CALLBACK_DO_NACK
            if(frame.fcf.ack_required) {
              do_nack = TSCH_CALLBACK_DO_NACK(current_link,
                  &source_address, &destination_address);
            }
#endif

            if(frame.fcf.ack_required) {
              static uint8_t ack_buf[TSCH_PACKET_MAX_LEN];
              static int ack_len;

              /* Build ACK frame */
              ack_len = tsch_packet_create_eack(ack_buf, sizeof(ack_buf),
                  &source_address, frame.seq, (int16_t)RTIMERTICKS_TO_US(estimated_drift), do_nack);

#if TSCH_SECURITY_ENABLED
              if(tsch_is_pan_secured) {
                /* Secure ACK frame. There is only header and header IEs, therefore data len == 0. */
                ack_len += tsch_security_secure_frame(ack_buf, ack_buf, ack_len, 0, &current_asn);
              }
#endif /* TSCH_SECURITY_ENABLED */

              /* Copy to radio buffer */
              NETSTACK_RADIO.prepare((const void *)ack_buf, ack_len);

              /* Wait for time to ACK and transmit ACK */
              TSCH_SCHEDULE_AND_YIELD(pt, t, rx_start_time,
                  packet_duration + tsch_timing[tsch_ts_tx_ack_delay] - RADIO_DELAY_BEFORE_TX, "RxBeforeAck");
              TSCH_DEBUG_RX_EVENT();
              NETSTACK_RADIO.transmit(ack_len);
            }

            /* If the sender is a time source, proceed to clock drift compensation */
            n = tsch_queue_get_nbr(&source_address);
            if(n != NULL && n->is_time_source) {
              int32_t since_last_timesync = ASN_DIFF(current_asn, last_sync_asn);
              /* Keep track of last sync time */
              last_sync_asn = current_asn;
              /* Save estimated drift */
              drift_correction = -estimated_drift;
              is_drift_correction_used = 1;
              tsch_timesync_update(n, since_last_timesync, -estimated_drift);
              tsch_schedule_keepalive();
            }

            /* Add current input to ringbuf */
            ringbufindex_put(&input_ringbuf);

            /* Log every reception */
            TSCH_LOG_ADD(tsch_log_rx,
              log->rx.src = TSCH_LOG_ID_FROM_LINKADDR((linkaddr_t*)&frame.src_addr);
              log->rx.is_unicast = frame.fcf.ack_required;
              log->rx.datalen = current_input->len;
              log->rx.drift = drift_correction;
              log->rx.drift_used = is_drift_correction_used;
              log->rx.is_data = frame.fcf.frame_type == FRAME802154_DATAFRAME;
              log->rx.sec_level = frame.aux_hdr.security_control.security_level;
              log->rx.estimated_drift = estimated_drift;
            );
          } else {
            TSCH_LOG_ADD(tsch_log_message,
                  snprintf(log->message, sizeof(log->message),
                      "!not for us %x:%x",
                      destination_address.u8[LINKADDR_SIZE - 2], destination_address.u8[LINKADDR_SIZE - 1]);
            );
          }

          /* Poll process for processing of pending input and logs */
          process_poll(&tsch_pending_events_process);
        }
      } else {
			int n_ts = (int)&current_asn % TSCH_SCHEDULE_DEFAULT_LENGTH;
			if(we_are_learning == STILL_LEARNING && n_ts){
				printf("slot %d: timeout, maybe collision, listen failed\n", n_ts);
				update_distribution_table(n_ts - 1, &linkaddr_null, 0);
				exploitation(n_ts - 1);
			}
      }
    }

    if(input_queue_drop != 0) {
      TSCH_LOG_ADD(tsch_log_message,
          snprintf(log->message, sizeof(log->message),
              "!queue full skipped %u", input_queue_drop);
      );
      input_queue_drop = 0;
    }
  }

  TSCH_DEBUG_RX_EVENT();

  PT_END(pt);
}
/*---------------------------------------------------------------------------*/
/* Protothread for slot operation, called from rtimer interrupt
 * and scheduled from tsch_schedule_slot_operation */
static
PT_THREAD(tsch_slot_operation(struct rtimer *t, void *ptr))
{
  TSCH_DEBUG_INTERRUPT();
  PT_BEGIN(&slot_operation_pt);

  /* Loop over all active slots */
  while(tsch_is_associated) {

    if(current_link == NULL || tsch_lock_requested) { /* Skip slot operation if there is no link
                                                          or if there is a pending request for getting the lock */
      /* Issue a log whenever skipping a slot */
      TSCH_LOG_ADD(tsch_log_message,
                      snprintf(log->message, sizeof(log->message),
                          "!skipped slot %u %u %u",
                            tsch_locked,
                            tsch_lock_requested,
                            current_link == NULL);
      );

    } else {
      uint8_t current_channel;
      TSCH_DEBUG_SLOT_START();
      tsch_in_slot_operation = 1;
	  int n_ts = (int)&current_asn % TSCH_SCHEDULE_DEFAULT_LENGTH;
      /* Get a packet ready to be sent */
	  if (we_are_learning != LEARNING_DONE) {
	  	current_packet = get_packet_and_neighbor_for_link(current_link, &current_neighbor, &current_asn);
	  } else {
	  	current_packet = get_packet_and_neighbor_for_link(current_link, &strategy[n_ts], &current_asn);
	  }
	  
      /* There is no packet to send, and this link does not have Rx flag. Instead of doing
       * nothing, switch to the backup link (has Rx flag) if any. */
      if(current_packet == NULL && !(current_link->link_options & LINK_OPTION_RX) && backup_link != NULL) {
        current_link = backup_link;
  	  	if (we_are_learning != LEARNING_DONE) {
  	  		current_packet = get_packet_and_neighbor_for_link(current_link, &current_neighbor, &current_asn);
  	  	} else {
  	  		current_packet = get_packet_and_neighbor_for_link(current_link, &strategy[n_ts], &current_asn);
  	  	}
      }
      /* Hop channel */
      current_channel = tsch_calculate_channel(&current_asn, current_link->channel_offset);
      NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, current_channel);
      /* Reset drift correction */
      drift_correction = 0;
      is_drift_correction_used = 0;
      /* Decide whether it is a TX/RX/IDLE or OFF slot */
      /* Actual slot operation */
      if(current_packet != NULL) {
        /* We have something to transmit, do the following:
         * 1. send
         * 2. update_backoff_state(current_neighbor)
         * 3. post tx callback
         **/
        static struct pt slot_tx_pt;
        PT_SPAWN(&slot_operation_pt, &slot_tx_pt, tsch_tx_slot(&slot_tx_pt, t));
      } else if((current_link->link_options & LINK_OPTION_RX)) {
        /* Listen */
        static struct pt slot_rx_pt;
        PT_SPAWN(&slot_operation_pt, &slot_rx_pt, tsch_rx_slot(&slot_rx_pt, t));
      }
      TSCH_DEBUG_SLOT_END();
    }

    /* End of slot operation, schedule next slot or resynchronize */

    /* Do we need to resynchronize? i.e., wait for EB again */
    if(!tsch_is_coordinator && (ASN_DIFF(current_asn, last_sync_asn) >
        (100 * TSCH_CLOCK_TO_SLOTS(TSCH_DESYNC_THRESHOLD / 100, tsch_timing[tsch_ts_timeslot_length])))) {
      TSCH_LOG_ADD(tsch_log_message,
            snprintf(log->message, sizeof(log->message),
                "! leaving the network, last sync %u",
                          (unsigned)ASN_DIFF(current_asn, last_sync_asn));
      );
      last_timesource_neighbor = NULL;
      tsch_disassociate();
    } else {
      /* backup of drift correction for printing debug messages */
      /* int32_t drift_correction_backup = drift_correction; */
      uint16_t timeslot_diff = 0;
      rtimer_clock_t prev_slot_start;
      /* Time to next wake up */
      rtimer_clock_t time_to_next_active_slot;
      /* Schedule next wakeup skipping slots if missed deadline */
      do {
        if(current_link != NULL
            && current_link->link_options & LINK_OPTION_TX
            && current_link->link_options & LINK_OPTION_SHARED) {
          /* Decrement the backoff window for all neighbors able to transmit over
           * this Tx, Shared link. */
          tsch_queue_update_all_backoff_windows(&current_link->addr);
        }

        /* Get next active link */
        current_link = tsch_schedule_get_next_active_link(&current_asn, &timeslot_diff, &backup_link);
        if(current_link == NULL) {
          /* There is no next link. Fall back to default
           * behavior: wake up at the next slot. */
          timeslot_diff = 1;
        }
        /* Update ASN */
        ASN_INC(current_asn, timeslot_diff);
        /* Time to next wake up */
        time_to_next_active_slot = timeslot_diff * tsch_timing[tsch_ts_timeslot_length] + drift_correction;
        drift_correction = 0;
        is_drift_correction_used = 0;
        /* Update current slot start */
        prev_slot_start = current_slot_start;
        current_slot_start += time_to_next_active_slot;
        current_slot_start += tsch_timesync_adaptive_compensate(time_to_next_active_slot);
      } while(!tsch_schedule_slot_operation(t, prev_slot_start, time_to_next_active_slot, "main"));
    }

    tsch_in_slot_operation = 0;
	
	cycle_check_learning();
	/* AL-MMAC learning done */
    if((num_learning_done >= MAX_LEARNING_SLOTS) && (we_are_learning != LEARNING_DONE)){
    	we_are_learning = LEARNING_DONE;
    	learning_fix_result();
		print_distribution_table();
    }
	
    PT_YIELD(&slot_operation_pt);
  }

  PT_END(&slot_operation_pt);
}
/*---------------------------------------------------------------------------*/
/* Set global time before starting slot operation,
 * with a rtimer time and an ASN */
void
tsch_slot_operation_start(void)
{
  static struct rtimer slot_operation_timer;
  rtimer_clock_t time_to_next_active_slot;
  rtimer_clock_t prev_slot_start;
  TSCH_DEBUG_INIT();
  do {
    uint16_t timeslot_diff;
    /* Get next active link */
    current_link = tsch_schedule_get_next_active_link(&current_asn, &timeslot_diff, &backup_link);
    if(current_link == NULL) {
      /* There is no next link. Fall back to default
       * behavior: wake up at the next slot. */
      timeslot_diff = 1;
    }
    /* Update ASN */
    ASN_INC(current_asn, timeslot_diff);
    /* Time to next wake up */
    time_to_next_active_slot = timeslot_diff * tsch_timing[tsch_ts_timeslot_length];
    /* Update current slot start */
    prev_slot_start = current_slot_start;
    current_slot_start += time_to_next_active_slot;
  } while(!tsch_schedule_slot_operation(&slot_operation_timer, prev_slot_start, time_to_next_active_slot, "association"));
}
/*---------------------------------------------------------------------------*/
/* Start actual slot operation */
void
tsch_slot_operation_sync(rtimer_clock_t next_slot_start,
    struct asn_t *next_slot_asn)
{
  current_slot_start = next_slot_start;
  current_asn = *next_slot_asn;
  last_sync_asn = current_asn;
  current_link = NULL;
}
/*---------------------------------------------------------------------------*/
