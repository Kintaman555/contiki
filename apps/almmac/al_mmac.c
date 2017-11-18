/*
 * al_mmac.c
 *
 *  Created on: Jan 11, 2012
 *
 *  Ver Oct 31: renew
 */

#include "contiki.h"

#include "sys/pt.h"
#include "sys/rtimer.h"
#include "lib/memb.h"
#include "lib/list.h"
#include "lib/random.h"

#include "net/rime/rime.h"
#include "net/mac/mac.h"
#include "net/mac/rdc.h"
#include "net/netstack.h"
#include "net/rime/timesynch.h"
//#include "net/neighbor-attr.h"
#include "net/rime/collect-neighbor.h"
#include "net/queuebuf.h"
#include "net/packetbuf.h"

#include "al_mmac.h"

#include "dev/cc2420/cc2420.h"
#include <stdio.h>

#if DEBUG
...
#else
#include "dev/leds.h"
#undef LEDS_ON
#undef LEDS_OFF
#define LEDS_ON(leds)    //leds_on(leds)
#define LEDS_OFF(leds)   //leds_off(leds)
#define PRINTF(...)
#endif

#define DEBUG_LEARNING 0
#if DEBUG_LEARNING
#define PRINTFL(...) printf(__VA_ARGS__)
#else
#define PRINTFL(...)
#endif

#define last_arrival() RTIMER_TO_LONG(cc2420_time_of_arrival)

/* XXX Currently we input packet to upper layer after each received packet.
 * We could also receive all packets and then input them subsequently to the upper
 * layers. This would improve throughput speed of a packet train/stream, BUT
 * will also require significant additional buffer space...
 */

#define MAX_QUEUED_PACKETS  10
#define QUEUED_PACKET_LIFETIME	(SLOTS_PER_FRAME + 1)

#define WITH_REXMIT         1  /* 0 disables MAC-layer rxmits, even if PACKETBUF_ATTR_MAX_REXMIT is set */
#define DEFAULT_MAX_TXMITS  4  /* including first transmission
                                  if WITH_REXMIT is set and PACKETBUF_ATTR_MAX_REXMIT is not set (in collect???)*/

#define U8(a) (a)->u8[0],(a)->u8[1] /* for printf */
#ifdef COLLECT_NEIGHBOR_CONF_MAX_COLLECT_NEIGHBORS
#undef MAX_NEIGHBORS
#define MAX_NEIGHBORS COLLECT_NEIGHBOR_CONF_MAX_COLLECT_NEIGHBORS
#else /* COLLECT_NEIGHBOR_CONF_MAX_COLLECT_NEIGHBORS */
#undef MAX_NEIGHBORS
#define MAX_NEIGHBORS 8
#endif /* COLLECT_NEIGHBOR_CONF_MAX_COLLECT_NEIGHBORS */

/*------------------======== timing definitions ==========-------------------*/
/* receiver side */
#define ASYNCH_MARGIN	 (RTIMER_SECOND / 512) // wait a bit before send beacon
#define CONTENTION_WAIT  (RTIMER_SECOND / 1024) // OFF radio to wait for transmitter contending channel
#define CCA_DELAY        (RTIMER_SECOND / 1024)//1100 // wait before ON radio to check cca
#define CCA_CHECK_TIME   (RTIMER_SECOND / 2048) // cca check duration
#define CCA_CHECK_NUM_UC  2
#define CCA_CHECK_NUM_BC  2
#define MSG_WAIT         (RTIMER_SECOND / 200)

/* transmitter side */
#define BEACON_WAIT  	 (RTIMER_SECOND / 128) // 400 vs 1024; 256 vs 512 // ON radio to wait for receiving beacon
#define ACK_WAIT         (RTIMER_SECOND / 200) // ON radio to wait for ack after transmitting packet

#define PREDATA_NUM       1
#define PREDATA_SIZE      72  /* (bytes) long enough to cover CCA_DELAY */
#define SEND_BEACON_IN_BC_SLOT   1 // to send beacon after finishing sending broadcast mes in broadcast slot
#define UC_OVER_BC        2 /* broadcast slot frequency */
#define RANDOM_SYNCH      0 /* (1/RANDOM_SYNCH) chance of random wake-up, disabled if 0 */

#define SLOT_INDEX(t)	((timesynch_rtimer_to_time(t) / SLOT_TIME) % (SLOTS_PER_FRAME + 1)) //plus 1 for broadcast slot

/*---------------======== for learning process ==========-----------------*/
#define FRAMES_START_EXPLOITATION (30 * SLOTS_PER_FRAME)
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

struct learning_distribution {
	u8_t successful_slot;
	u8_t total_slot;
	u16_t successful_prob;
	int num; // num of check cca when contention, depends on # successful trials
};
struct learning_distributions {
	struct learning_distributions *next;
	struct learning_distribution distributions[SLOTS_PER_FRAME];
	linkaddr_t parent;
};
MEMB(learning_distribution_mem, struct learning_distributions, MAX_NEIGHBORS);
LIST(distribution_list);
struct parent {
	struct parent *next;
	linkaddr_t addr;
};
MEMB(parent_mem, struct parent, MAX_NEIGHBORS);
LIST(parent_list);
static linkaddr_t strategy[SLOTS_PER_FRAME];
static struct collect_neighbor_list neighbor_list;
static u16_t my_rtmetric;

/*---------------======== packet type definitions ==========-----------------*/
enum mac_type {
  TYPE_BEACON,
  TYPE_PREDATA,
  TYPE_DATA,
  TYPE_ACK
};
struct beacon {
  /* beacon type packet doesn't need a receiver */
  struct type_hdr type;
  linkaddr_t sender;
  u16_t           authority;
  rtimer_clock_t   send_time;
};
struct ack {
  //struct much_hdr hdr; //much_hdr is defined in file  .h for chameleon
  struct type_hdr type;
  linkaddr_t sender;
  linkaddr_t receiver; //Huy added
  u16_t           authority;
  rtimer_clock_t   send_time;
};
/*---------------======== packet queue definitions ==========----------------*/
struct queue_item {
  struct queuebuf* packet;
  linkaddr_t ereceiver;
  linkaddr_t receiver;
  u8_t sent;
  u8_t latency;//Huy
  u8_t txmits;
  u8_t max_txmits;
  //u8_t epkt_id;
  mac_callback_t sent_callback;
  void *sent_callback_ptr;
};
MEMB(queue_memb, struct queue_item, MAX_QUEUED_PACKETS);
volatile struct queue_item* pending_bc = NULL;
/*---------------------------------------------------------------------------*/
//mode type and learning status
enum {
	LISTEN_MODE,
	TRANSMIT_MODE,
	SLEEP_MODE,
	NOT_YET_LEARNING,
	STILL_LEARNING,
	LEARNING_DONE
};
/*---------------------------------------------------------------------------*/
PROCESS(dutycycle_process, "almmac dutycycle process");
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
	memb_init(&learning_distribution_mem);
	list_init(distribution_list);
	memb_init(&parent_mem);
	list_init(parent_list);
	int i;
	struct collect_neighbor *n;
	struct learning_distributions *ld;
	//create list of candidate parent from neighbor list, initialize distribution list
	for(n = list_head(neighbor_list.list); n != NULL; n = list_item_next(n)) {
		//printf("neighbor %d, n->rtmetric %d ", n->addr.u8[0],n->rtmetric);
		if((n->rtmetric + 1) == my_rtmetric){
			struct parent *p = memb_alloc(&parent_mem);
			ld = memb_alloc(&learning_distribution_mem);
			linkaddr_copy(&p->addr, &n->addr);
			linkaddr_copy(&ld->parent, &n->addr);
			for(i = 0; i < SLOTS_PER_FRAME; i++){
				ld->distributions[i].num = MAX_CHECK_CCA >> 1;
				ld->distributions[i].successful_slot = 0;
				ld->distributions[i].total_slot = 0;
			}
			list_add(parent_list, p);
			list_add(distribution_list, ld);
			//printf("add to parent list %d", p->addr.u8[0]);
		}
		//printf("\n");
	}
	//for listen trial
	ld = memb_alloc(&learning_distribution_mem);
	linkaddr_copy(&ld->parent, &linkaddr_null);
	for(i = 0; i < SLOTS_PER_FRAME; i++){
		ld->distributions[i].num = 0;
		ld->distributions[i].successful_slot = 0;
		ld->distributions[i].total_slot = 0;
	}
	list_add(distribution_list, ld);
	//randomly initialize strategy
	for(i = 0; i < SLOTS_PER_FRAME; i++){
	  int index = random_rand() % (list_length(parent_list) + 1);
	  if(index == list_length(parent_list)){
	    linkaddr_copy(&strategy[i], &linkaddr_null);
	  } else {
		int j = 0;
		struct parent *p = NULL;
		for(p = list_head(parent_list); p != NULL; p = list_item_next(p)){
	      if(j == index){
	    	  linkaddr_copy(&strategy[i], &p->addr);
	    	  break;
	      }
	      j++;
		}
	  }
	  //printf("slot %d, parent %d \n",i,  strategy[i].u8[0]);
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
  for(i = 0; i < SLOTS_PER_FRAME; i ++){
	struct learning_distributions *ld;
	for(ld = list_head(distribution_list); ld != NULL; ld = list_item_next(ld)){
		PRINTFL("slot %d, parent %d, total %d, success %d\n",
				i, ld->parent.u8[0],
				ld->distributions[i].total_slot,
				ld->distributions[i].successful_slot);

	}
  }
}
/*---------------------------------------------------------------------------*/
static void update_distribution_table(u8_t slot, linkaddr_t *parent, u8_t successful)
{
  struct learning_distributions *ld;
  for(ld = list_head(distribution_list); ld != NULL; ld = list_item_next(ld)){
	if(linkaddr_cmp(&ld->parent, parent)){
	  ld->distributions[slot].total_slot ++;
	  if(successful){
		ld->distributions[slot].successful_slot ++;
	  }
	  if(!linkaddr_cmp(parent, &linkaddr_null)){
		u16_t fail, total;
		total = ld->distributions[slot].total_slot;
		fail = total - ld->distributions[slot].successful_slot;
		ld->distributions[slot].num = (MAX_CHECK_CCA * (fail + 1)) / (total + 2);
		//printf("%d, %d => %d\n", success, total, ld->distributions[slot].num);
	  }
	  //PRINTFL("slot %d,parent %d, %d, %d => %d\n", slot, parent->u8[0], ld->distributions[slot].successful_slot,
		//	  ld->distributions[slot].total_slot , ld->distributions[slot].num);
	  return;
	}
  }
  //printf("parent %d not found\n", parent->u8[0]);
  return;
}
/*---------------------------------------------------------------------------*/
static int check_cca_num(u8_t slot, linkaddr_t *parent)
{
  if(we_are_learning == STILL_LEARNING){
	struct learning_distributions *ld;
	for(ld = list_head(distribution_list); ld != NULL; ld = list_item_next(ld)){
	  if(linkaddr_cmp(&ld->parent, parent)){
		if(ld->distributions[slot].num < WIN_THRESHOLD){
			return 0;
		}
		if(ld->distributions[slot].num > LOSE_THRESHOLD){
			return MAX_CHECK_CCA - 1;
		}
		return ld->distributions[slot].num;
	  }
	}
  }
  return 2;
}
/*---------------------------------------------------------------------------*/
static void learning_fix_result(void)
{
  int i;
  struct learning_distributions *ld;
  struct parent *p;
  for(i = 0; i < SLOTS_PER_FRAME; i++){
	u16_t total, success, prob, max_prob; //must be u16_t (larger than type of total_slot)
	total = 0, max_prob = 0;
	struct learning_distributions *best_ld;
	best_ld = NULL;
	//# trials of slot
    for(ld = list_head(distribution_list); ld != NULL; ld = list_item_next(ld)){
      total += ld->distributions[i].total_slot;
    }
    //find the parent which has the most successful prob
    for(ld = list_head(distribution_list); ld != NULL; ld = list_item_next(ld)){
      success = ld->distributions[i].successful_slot;
      prob = (total) ? ((success * PROBABILITY_SCALE) / total) : 0;
      if(prob > max_prob){
        max_prob = prob;
        best_ld = ld;
      }
    }
    // setting strategy
    if(best_ld == NULL){ //all probs = 0 || no item in distribution list
      linkaddr_copy(&strategy[i], &sleep_addr);
      //printf("slot %d, best_ld == NULL ==> setting strategy sleep %d\n", i+1, strategy[i].u8[0]);
    }else{
      if(max_prob >= SUCCESS_PROBABILITY){
        linkaddr_copy(&strategy[i], &best_ld->parent);
        //leds_on(LEDS_YELLOW);
      } else {
        linkaddr_copy(&strategy[i], &sleep_addr);
      }
    /*  printf("slot %d, parent %d, max_prob %d, total %d ==>  setting strategy, parent %d \n", i+1,
          			best_ld->parent.u8[0], max_prob, total, strategy[i].u8[0]);*/
      printf("slot\t%d\ts\t%d\tp\t%d\ttotal\t%d\tset\t%d\n", i+1,
                			best_ld->parent.u8[0], max_prob, total, strategy[i].u8[0]);
    }
  }
  //free mem of learning distribution and parent list
  for(ld = list_head(distribution_list); ld != NULL; ld = list_item_next(ld)){
	memb_free(&learning_distribution_mem, ld);
  }
  for(p = list_head(parent_list); p != NULL; p = list_item_next(p)){
	memb_free(&parent_mem, p);
  }
}
/*---------------------------------------------------------------------------*/
static void exploitation(u8_t slot)
{
#if RANDOM_CHOOSE_PARENT
	//printf("Exploitation: slot %d, old %d, new", slot, strategy[slot].u8[0]);
	int index = random_rand() % (list_length(parent_list) + 1);
	if(index == list_length(parent_list)){
	  linkaddr_copy(&strategy[slot], &linkaddr_null);
	} else {
	  int j = 0;
	  struct parent *p = NULL;
	  for(p = list_head(parent_list); p != NULL; p = list_item_next(p)){
		if(j == index){
		  linkaddr_copy(&strategy[slot], &p->addr);
		  break;
		}
		j++;
	  }
	}
	//printf(" %d\n", strategy[slot].u8[0]);
#else
  if(num_learning_done < FRAMES_START_EXPLOITATION){ //randomly choose parent in the first 30 frames
	int index = random_rand() % (list_length(parent_list) + 1);
	if(index == list_length(parent_list)){
		linkaddr_copy(&strategy[slot], &linkaddr_null);
	} else {
		int j = 0;
		struct parent *p = NULL;
		for(p = list_head(parent_list); p != NULL; p = list_item_next(p)){
			if(j == index){
				linkaddr_copy(&strategy[slot], &p->addr);
				break;
			}
			j++;
		}
	}
	return;
  }
/*
  struct learning_distributions *ld;
  u16_t total_prob = 0, recent_prob = 0;
  u8_t seed;

  for(ld = list_head(distribution_list); ld != NULL; ld = list_item_next(ld)){
	  ld->distributions[slot].successful_prob = (PROBABILITY_SCALE * (ld->distributions[slot].successful_slot + 1)) /
			(ld->distributions[slot].total_slot + 1);
	  PRINTFL("parent %d, success %d, total %d, prob %d\n", ld->parent.u8[0], ld->distributions[slot].successful_slot,
			  ld->distributions[slot].total_slot, ld->distributions[slot].successful_prob);
	  total_prob += ld->distributions[slot].successful_prob;
  }

  for(ld = list_head(distribution_list); ld != NULL; ld = list_item_next(ld)){
	  recent_prob += (PROBABILITY_SCALE * ld->distributions[slot].successful_prob) / total_prob;
	  ld->distributions[slot].successful_prob = recent_prob; //standardize
	  PRINTFL("parent %d, standardize prob %d\n", ld->parent.u8[0], ld->distributions[slot].successful_prob);
  }

  seed = random_rand() % PROBABILITY_SCALE;
  PRINTFL("seed %d =>", seed);
  for(ld = list_head(distribution_list); ld != NULL; ld = list_item_next(ld)){
	  if(ld->distributions[slot].successful_prob > seed){
		  linkaddr_copy(&strategy[slot], &ld->parent);
		  break;
	  }
  }
  return;*/
	 struct learning_distributions *ld, *best_ld = NULL;
	 u16_t total = 0, prob, max_prob = 0;

	 for(ld = list_head(distribution_list); ld != NULL; ld = list_item_next(ld)){
		 prob = ((2 + ld->distributions[slot].successful_slot) * PROBABILITY_SCALE) / (ld->distributions[slot].total_slot + 4);
		 if(prob > max_prob){
			 max_prob = prob;
			 best_ld = ld;
		 }
	 }
	 //printf("Exploitation: slot %d, old %d,", slot, strategy[slot].u8[0]);
	 if(best_ld == NULL || (max_prob < EXPLOITATION_PROBABILITY)){ //hack
		 int index = random_rand() % (list_length(parent_list) + 1);
		 if(index == list_length(parent_list)){
			 linkaddr_copy(&strategy[slot], &linkaddr_null);
		 } else {
			 int j = 0;
			 struct parent *p = NULL;
			 for(p = list_head(parent_list); p != NULL; p = list_item_next(p)){
				 if(j == index){
					 linkaddr_copy(&strategy[slot], &p->addr);
					 break;
				 }
				 j++;
			 }
		 }
		 //printf(" random choose %d\n", strategy[slot].u8[0]);
	 }else{
		 linkaddr_copy(&strategy[slot], &best_ld->parent);
		 //printf(" new %d\n", strategy[slot].u8[0]);
	 }
#endif
}
/*---------------------------------------------------------------------------*/
static struct ctimer cycle_check_timer;
#define CHECK_LEARNING_PERIOD (CLOCK_SECOND * 2)
/* #frames learning, must less than type of total_slot to avoid overflow error,
 * currently use u8_t => max value = 255 */
#define MAX_LEARNING_SLOTS	(SLOTS_PER_FRAME * 150)
static void cycle_check_learning(void)
{
	if(num_learning_done >= MAX_LEARNING_SLOTS){
		we_are_learning = LEARNING_DONE;
		learning_fix_result();
		//leds_on(LEDS_YELLOW);
		//print_distribution_table();
	}
	if(we_are_learning == STILL_LEARNING){
		ctimer_set(&cycle_check_timer, CHECK_LEARNING_PERIOD,(void *) cycle_check_learning, NULL);
	}
}
/*---------------------------------------------------------------------------*/
static void free_queue(void);
/*---------------------------------------------------------------------------*/
void learning_start(void){
	//printf("learning start\n");
	num_learning_done = 0;
	learning_init();
	free_queue();
	we_are_learning = STILL_LEARNING;
	//cycle_check_learning();
}
/*---------------------------------------------------------------------------*/
static void on(void)
{
  if (radio_is_on == 0) {
    radio_is_on = 1;
    NETSTACK_RADIO.on();
    //LEDS_ON(LEDS_RED);
  }
}
/*---------------------------------------------------------------------------*/
static void off(void)
{
  if (radio_is_on != 0) {
    radio_is_on = 0;
    NETSTACK_RADIO.off();
    //LEDS_OFF(LEDS_ALL);
  }
}
/*---------------------------------------------------------------------------*/
static void
dequeue_packet(struct queue_item* item)
{
   /* clear packet */
  queuebuf_free(item->packet);
  memb_free(&queue_memb, item);
}
/*---------------------------------------------------------------------------*/
// free all packets in queue before learning
static void free_queue(void){
	int i;
	struct queue_item* items = queue_memb.mem;
	for (i = 0; i < queue_memb.num; i++){
	  if (queue_memb.count[i]){
		  memb_free(&queue_memb, &items[i]);
	  }
	}
	return;
}
/*---------------------------------------------------------------------------*/
static void queue_update_latency(void)
{
  int i;
  struct queue_item* items = queue_memb.mem;
  for (i = 0; i < queue_memb.num; i++){
	if (queue_memb.count[i]){
	  items[i].latency ++;
	  if(we_are_learning != NOT_YET_LEARNING/*== STILL_LEARNING*/){
		// free packet which is past its lifetime
		if(items[i].latency > QUEUED_PACKET_LIFETIME){
			dequeue_packet(&items[i]);
		}
	  }
	}
  }
}
/*---------------------------------------------------------------------------*/
static unsigned int
queued_packets(void)
{
  int i;
  unsigned int cnt = 0;
  for (i = 0; i < queue_memb.num; i++){
    if (queue_memb.count[i]){
      cnt++;
    }
  }
  return cnt;
}
/*---------------------------------------------------------------------------*/
static linkaddr_t*
queued_unicast_packets_for(void)
{
  int i;
  struct queue_item* items = queue_memb.mem;
  for (i = 0; i < queue_memb.num; i++){
    if (queue_memb.count[i]){
      struct much_hdr* hdr = queuebuf_dataptr(items[i].packet);
      if (!linkaddr_cmp(&(hdr->receiver), &linkaddr_null)){
        return &(hdr->receiver);
      }
    }
  }
  return (&linkaddr_null);
}
/*-----------------------------------------------------------------*/
static struct queue_item*
queued_item_for(linkaddr_t *dest)
{
  int i;
  struct queue_item* items = queue_memb.mem;
  for (i = 0; i < queue_memb.num; i++){
	if (queue_memb.count[i]){
	  struct much_hdr* hdr = queuebuf_dataptr(items[i].packet);
		if(!linkaddr_cmp(&(hdr->receiver), &linkaddr_null)) {
			//printf("queue: %d\n", hdr->receiver.u8[0]);
			return &(items[i]);
		}
	}
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
static void
send_beacon(void)
{ // in listen mode, node sends beacon after waking-up
  struct beacon beacon;
  linkaddr_copy(&(beacon.sender), &linkaddr_node_addr);
  beacon.type.ptype = TYPE_BEACON;
  beacon.type.seq = !(queued_packets() == MAX_QUEUED_PACKETS);
  beacon.authority = timesynch_authority_level();
  beacon.send_time = timesynch32_time();

  NETSTACK_RADIO.send(&beacon, sizeof(beacon));
  timesynch_synch_delay(timesynch32_time() - beacon.send_time);
}
/*---------------------------------------------------------------------------*/
static void
send_pending_bc(void)
{ // node sends broadcast packets in broadcast slot
  int time_check_cca = 2048 / (1 + random_rand() % 4);
  int incoming = 0;
  rtimer_clock_t t0;

  on();
  t0 = RTIMER_NOW();
  while(!NETSTACK_RADIO.pending_packet() && RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + (RTIMER_SECOND / time_check_cca)));
  incoming = (NETSTACK_RADIO.channel_clear() == 0) || NETSTACK_RADIO.pending_packet();
  if(incoming){
	  off();
  } else {
    char buf[PREDATA_SIZE];
    ((struct type_hdr*)buf)->ptype = TYPE_PREDATA;
    NETSTACK_RADIO.send(buf, PREDATA_SIZE);
    struct queue_item* next_message = NULL;
    /* look for BC messages */
    int i;
    struct queue_item* items = queue_memb.mem;
    for (i = 0; i < queue_memb.num; i++){
      if (queue_memb.count[i]){
    	struct queue_item* item = &(items[i]);
    	if (item != pending_bc){
    		struct much_hdr* hdr = queuebuf_dataptr(item->packet);
    		if (linkaddr_cmp(&(hdr->receiver),&linkaddr_null)){
    			next_message = item;
    			break;
    		}
    	}
      }
    }
    ((struct much_hdr*)queuebuf_dataptr(pending_bc->packet))->type.seq = (next_message != NULL);
    NETSTACK_RADIO.send(queuebuf_dataptr(pending_bc->packet), queuebuf_datalen(pending_bc->packet));
    off();
#if SEND_BEACON_IN_BC_SLOT
    t0 = RTIMER_NOW();
    /*first, we wait for the receiver handling the broadcast message, afterwards, we send a beacon*/
    while(RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + (RTIMER_SECOND / 512)));
    on();
    send_beacon();
    off();
#endif
    dequeue_packet(pending_bc);
    pending_bc = next_message;
  }
}
/*---------------------------------------------------------------------------*/
static int
needs_resynch(void)
{
  if (timesynch_authority_level() == 0){
    return 0;
  }
#if RANDOM_SYNCH
  if (((u16_t)rand()) % RANDOM_SYNCH > 0){
    return 2;
  }
#endif

  return ((clock_seconds() - timesynch_last_correction() > 30 && !(rand()%15))
      || (timesynch_last_correction() == 0));
}
/*---------------------------------------------------------------------------*/
/* linear feedback shift register */
static u16_t pseudorandom(u16_t flips, const linkaddr_t* node)
{
 /* int i;
  flips++; // this way flips > 0
  u16_t seed = *(u16_t*)node;
  for (i=0; i < flips; i++){
    u16_t bit = ((seed >> 0) ^ (seed >> 12) ^ (seed >> 13) ^ (seed >> 15));
    seed = (seed >> 1) | (bit << 15);
  }*/
  u16_t id = node->u8[0];//*(u16_t*)node;//
  int i;
  for(i = 0; i <= flips; i++){
	  id = (u16_t) ((u32_t) id * /*16807*/25173 + 13849);
  }
  return (id % (SLOTS_PER_FRAME + 1));
}
/*---------------------------------------------------------------------------*/
//all nodes are scheduled to wake-up nearly at the same time
static rtimer_clock_t
next_wakeup(const linkaddr_t* node)
{
  rtimer_clock_t now = timesynch32_time();
  rtimer_clock_t wakeup = (rtimer_clock_t) ((now / SLOT_TIME) * SLOT_TIME + SLOT_TIME);
  rtimer_clock_t wakeup_short = (rtimer_clock_t) (wakeup & 0xFFFF);
  return timesynch_time_to_rtimer(wakeup_short);
}
/*---------------------------------------------------------------------------*/
static inline int
is_broadcast(rtimer_clock_t t)
{
  return (SLOT_INDEX(t) == 0);
}
/*---------------------------------------------------------------------------*/
static void set_radio_channel(int channel)
{
  //int channel = 26; //printf("channel %u\n", channel);
  if (cc2420_get_channel() != channel) {
    cc2420_set_channel(channel);
  }
}
/*---------------------------------------------------------------------------*/
#define NUM_FREQS 8//
const u16_t freqs[NUM_FREQS] = {11, 15, 20, 26, 15, 20, 26};//{11, 14, 15, 16, 19, 20, 25, 26};//
/*---------------------------------------------------------------------------*/
static unsigned short
get_tx_channel(rtimer_clock_t rnow, const linkaddr_t* target)
{
  rtimer_clock_t now = timesynch_rtimer_to_time(rnow);
  u16_t slot_index = (now / SLOT_TIME);
/*  u16_t seed = *((u16_t*)target);
  u16_t fi = (slot_index + seed) % NUM_FREQS;
  return freqs[fi];*/
  u16_t seed = pseudorandom(slot_index, target) % 7/*NUM_FREQS*/;
  return freqs[seed];
}
/*---------------------------------------------------------------------------*/
static void input_packet(void);                   // forward;
static struct rtimer rt;
static void run_dutycycle(struct rtimer *t, void* ptr)
{
  process_poll(&dutycycle_process);
}
/*---------------------------------------------------------------------------*/
static void set_transmit_mode(linkaddr_t *to)
{
	mode = TRANSMIT_MODE;
	set_radio_channel(get_tx_channel(RTIMER_TIME(&rt), to));
}
/*---------------------------------------------------------------------------*/
static void set_listen_mode(void)
{
	mode = LISTEN_MODE;
	set_radio_channel(get_tx_channel(RTIMER_TIME(&rt), &linkaddr_node_addr));
}
/*---------------------------------------------------------------------------*/
static void set_activity(u8_t slot){
#if BC_ALL_SLOTS_BEFORE_LEARNING
 if(we_are_learning != NOT_YET_LEARNING){
#endif
  queue_update_latency();
  if(!slot){ //broadcast slot
    mode = (pending_bc != NULL) ? TRANSMIT_MODE : LISTEN_MODE;
    set_radio_channel(26/*((timesynch_time()/SLOT_TIME)%16)+11*/);
  } else {
	slot--;
    linkaddr_t *dest = queued_unicast_packets_for();
    switch(we_are_learning){
    case STILL_LEARNING: {
      if((linkaddr_cmp(&strategy[slot], &linkaddr_null)) || (linkaddr_cmp(dest, &linkaddr_null))) {
    	set_listen_mode();
      }else{
    	set_transmit_mode(&strategy[slot]);
      }
      num_learning_done ++;
      break;
    }
    case LEARNING_DONE: {
      if(linkaddr_cmp(&strategy[slot], &sleep_addr)){
    	mode = SLEEP_MODE;
      }else{
    	if(linkaddr_cmp(&strategy[slot], &linkaddr_null)){
    	  set_listen_mode();
    	}else{
    	  set_transmit_mode(&strategy[slot]);
    	}
      }
      break;
    }
    default:{
      if(linkaddr_cmp(dest, &linkaddr_null)){
    	set_listen_mode();
      }else{
    	set_transmit_mode(dest);
      }
      break;
    }
    }
  }
  return;
#if BC_ALL_SLOTS_BEFORE_LEARNING
 }else{
	 sleep_cycle = 0;
	 mode = (pending_bc) ? TRANSMIT_MODE : LISTEN_MODE;
	 set_radio_channel(26/*((timesynch_time()/SLOT_TIME)%16)+11*/);
 }
#endif
}
/*---------------------------------------------------------------------------*/
//static volatile unsigned char beacon_recv = 0;
static volatile unsigned char need_synch = 0;
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(dutycycle_process, ev, data)
{
  static u16_t prev_channel;

  PROCESS_BEGIN();
  while(1){
    prev_channel = cc2420_get_channel();
    need_synch = needs_resynch();

  if(need_synch) {
	set_radio_channel(prev_channel);
	on();
  } else {
	volatile rtimer_clock_t t0 = RTIMER_NOW();
	u8_t slot = SLOT_INDEX(t0);
	set_activity(slot);
	if(mode != SLEEP_MODE){
	  if(mode == TRANSMIT_MODE){ //transmit mode
		if(!slot){
		  send_pending_bc();
		} else {
	//first, wait for beacon
  if(!learning_disable_beacon()){ //disable beacon when learning
    on();
    t0 = RTIMER_NOW();
    while (!NETSTACK_RADIO.pending_packet() && RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + BEACON_WAIT)); //wait for beacon from receiver
    off();

    if(NETSTACK_RADIO.pending_packet()){
    	//off();
      packetbuf_clear();
      int len = NETSTACK_RADIO.read(packetbuf_dataptr(), sizeof(struct beacon));
      if (len > 0){
    	packetbuf_set_datalen(len);
    	input_packet();
      }
      rtimer_clock_t end_of_beacon_wait = t0 + BEACON_WAIT;
      while (RTIMER_CLOCK_LT(RTIMER_NOW(), end_of_beacon_wait));
    }
  }else{
	  t0 = RTIMER_NOW();
	  while (RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + RTIMER_SECOND / 2000));
  }

    //send DATA
    struct queue_item* item = queued_item_for(&strategy[slot - 1]);
    if(item != NULL){//must be true
      int i;
      struct much_hdr* hdr = queuebuf_dataptr(item->packet);
      //contend channel
      int time_check_cca, max_check_cca;
      int incoming = 0;
      max_check_cca = (check_cca_num(slot - 1, &strategy[slot - 1]));
      if(max_check_cca)
      {
    	time_check_cca = 1 + random_rand() % max_check_cca;//2048/ (1 + random_rand() % max_check_cca);
    	on();
    	t0 = RTIMER_NOW();
    	while(!NETSTACK_RADIO.pending_packet() && RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + (time_check_cca * CCA_CHECK_TIME)/*(RTIMER_SECOND / time_check_cca)*/));
    	incoming = (!NETSTACK_RADIO.channel_clear()) || NETSTACK_RADIO.pending_packet();
      }

      if(incoming) {
    	off();
    	PRINTFL("slot %d, channel busy, transmission failed \n", slot);
    	if(we_are_learning == STILL_LEARNING){
    	  update_distribution_table(slot - 1, &strategy[slot - 1], 0);
    	  exploitation(slot - 1);
    	}
      } else { //channel is clear
    	/* send data preamble */
    	  char buf[PREDATA_SIZE];
    	  ((struct type_hdr*)buf)->ptype = TYPE_PREDATA;
    	  NETSTACK_RADIO.prepare(buf, PREDATA_SIZE);
    	  NETSTACK_RADIO.transmit(PREDATA_SIZE);
    	  hdr->type.seq = 1;
    	  // set end-to-end latency
    	  hdr->mac_latency = item->latency + queuebuf_attr(item->packet, PACKETBUF_ATTR_LATENCY);
    	  if(we_are_learning != NOT_YET_LEARNING){ //we must change receiver address in MAC header
		    linkaddr_copy(&(hdr->receiver), &strategy[slot - 1]);
		  }
    	  /* send the packet */
    	  NETSTACK_RADIO.send(queuebuf_dataptr(item->packet),	queuebuf_datalen(item->packet));
    	  off();
		  item->txmits++;
		  /* listen for ACK */
		  char got_ack = 0;
		  int len = 0;
		  struct ack ack;
		  t0 = RTIMER_NOW();
		  on();
		  while (!got_ack && RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + ACK_WAIT)){
			len = NETSTACK_RADIO.read(&ack, sizeof(struct ack));
			if (len > 0){
			  if (ack.type.ptype == TYPE_ACK && linkaddr_cmp(&(ack.receiver),&linkaddr_node_addr) &&
					  linkaddr_cmp(&(ack.sender), &(hdr->receiver))){
				off();
				got_ack = 1;
				item->sent = 1;  //printf("packet successfully sent! \n");
				//timesynch_incoming_packet(ack.send_time, last_arrival(), ack.authority);
			  } else {
				  //if something else => ack failed
				off();
				break; //while
			  }
			}
		  }
		  off();
		  if (!got_ack){
			if(we_are_learning == STILL_LEARNING){
			  PRINTFL("slot %d: not got ACK, transmission failed to %d\n", slot, strategy[slot - 1].u8[0]);
			  update_distribution_table(slot - 1, &strategy[slot - 1], 0);
			  exploitation(slot - 1);
			}
		  } else { // ACK received
			  //printf("slot %d: to %d\n",slot, strategy[slot - 1].u8[0]);
			  if(we_are_learning == STILL_LEARNING){
				update_distribution_table(slot - 1, &strategy[slot - 1], 1);
			  }
		  }//got ack
      }//channel is clear

      for (i = 0; i < queue_memb.num; i++){
    	if (queue_memb.count[i]){
    	  struct queue_item* items = queue_memb.mem;
    	  item = &(items[i]);
    	  if((item->sent == 1) /*|| (item->txmits >= item->max_txmits)*/){
    		  queuebuf_to_packetbuf(item->packet);
    		  packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &item->receiver);// not done?
    		  mac_call_sent_callback(item->sent_callback, item->sent_callback_ptr,
    				  item->sent == 1 ? MAC_TX_OK : MAC_TX_NOACK, item->txmits);
    		  dequeue_packet(item);
    	  }
    	}
      }//scan queue
    }// item != NULL
		}//end of transmit mode in unicast slot
	  } else { // listen mode

	int incoming = 0;
	if(slot ){ // unicast slot
	  t0 = RTIMER_NOW();
	  while (RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + ASYNCH_MARGIN)); // wait for a small margin
	  if(!learning_disable_beacon()){
	    int delay_time = 2048 / (1 + random_rand() % 8);
	    t0 = RTIMER_NOW();
	    while (RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + RTIMER_SECOND/delay_time));
	    on();
	    send_beacon();
	    off(); //from on() to off() is about 1 milisec
	    //t0 = RTIMER_NOW();
	    rtimer_clock_t end_of_beacon_wait = t0 + BEACON_WAIT - ASYNCH_MARGIN + RTIMER_SECOND/700;
	    while (RTIMER_CLOCK_LT(RTIMER_NOW(), end_of_beacon_wait));
	  }
	}
	// do CCA for check incoming packet
	unsigned int c;
	for (c = (slot ? CCA_CHECK_NUM_UC : CCA_CHECK_NUM_BC) ; (c) ; c--){
	  t0 = RTIMER_NOW();
	  while (RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + CCA_DELAY));
	  on();
	  t0 = RTIMER_NOW();
	  while (RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + CCA_CHECK_TIME));
	  incoming =  (NETSTACK_RADIO.pending_packet() || !NETSTACK_RADIO.channel_clear());
	  if (incoming){
		  listen_for_message = 1;
		  break; /* cca check*/
	  }
	  off();
	}

	watchdog_periodic(); // let wd know we are still alive
	if (incoming){
	  /* we need to account for the fact that we may receive PREDATA packets before the DATA packet arrives */
	  int timeout = 0;
	  while (listen_for_message&& !timeout){
		t0 = RTIMER_NOW();
		/* wait until input or timeout */
		while (!NETSTACK_RADIO.pending_packet() && (RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + MSG_WAIT)));
		if (NETSTACK_RADIO.pending_packet()){
			/* process message (bypasses cc2420 process)*/
			packetbuf_clear();
			int len = NETSTACK_RADIO.read(packetbuf_dataptr(), PACKETBUF_SIZE);
			if (len > 0){
			  packetbuf_set_datalen(len);
			  input_packet();
			}
		} else {/* timeout => probably a collision between incoming messages */
			timeout = 1;
			if(we_are_learning == STILL_LEARNING && slot){
				PRINTFL("slot %d: timeout, maybe collision, listen failed\n", slot);
				update_distribution_table(slot - 1, &linkaddr_null, 0);
				exploitation(slot - 1);
			}
		}
	  }
	  listen_for_message = 0;
	} else {//no packet received
		if(we_are_learning == STILL_LEARNING && slot){
			PRINTFL("slot %d: no incoming packet, listen failed\n", slot);
			update_distribution_table(slot - 1, &linkaddr_null, 0);
			exploitation(slot - 1);
		}
	}

	  }//end of listen mode
	}
	off();
	//printf("slot %d: %d \n", slot, get_tx_channel(RTIMER_TIME(&rt), &linkaddr_node_addr));
  }
  rtimer_set(&rt, next_wakeup(&linkaddr_node_addr), 1, run_dutycycle, NULL);
  if((num_learning_done >= MAX_LEARNING_SLOTS) && (we_are_learning != LEARNING_DONE)){
  	we_are_learning = LEARNING_DONE;
  	learning_fix_result();
  		//leds_on(LEDS_YELLOW);
  		//print_distribution_table();
  }

  PROCESS_YIELD();
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static u16_t bc_recv = 0;
static void
input_packet(void)
{
  rtimer_clock_t t0 = RTIMER_NOW();
  u8_t slot = SLOT_INDEX(t0);
if(packetbuf_totlen() > 0){
  switch (((struct type_hdr*)packetbuf_dataptr())->ptype){

  case TYPE_BEACON: {
	off();
    struct beacon* beacon = packetbuf_dataptr();
    timesynch_incoming_packet(beacon->send_time, last_arrival(), beacon->authority);
    break; /* switch */
  }

  case TYPE_DATA: {
	/*in UC slot, listen mode || BC slot, no BC packet*/
	listen_for_message = 0;
    off();
    struct much_hdr* hdr = packetbuf_dataptr();
    char is_broadcast = linkaddr_cmp(&(hdr->receiver),&linkaddr_null);
    if(!is_broadcast){
      if (linkaddr_cmp(&(hdr->receiver),&linkaddr_node_addr)) {
      /* for me, so send ACK, then notify upper layer */
        struct ack ack;
        ack.type.ptype = TYPE_ACK;
        ack.type.seq = 1;
        linkaddr_copy(&(ack.sender),&linkaddr_node_addr);
        linkaddr_copy(&(ack.receiver), &(hdr->sender));
        ack.authority = timesynch_authority_level();
        ack.send_time = timesynch32_time();
        NETSTACK_RADIO.send(&ack, sizeof(ack));
        //timesynch_synch_delay(timesynch32_time() - ack.send_time);
        //printf("slot %d: from %d! \n", slot, hdr->sender.u8[0]);
        if(we_are_learning == STILL_LEARNING){
          update_distribution_table(slot - 1, &linkaddr_null, 1);
        }
       /* if(we_are_learning == LEARNING_DONE){
        	printf("slot %d: from %d! \n", slot, hdr->sender.u8[0]);
        }*/
        packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &hdr->receiver); // not done?
        /* we do not reduce MAC header before forwarding packet to upper layer,
         * we need to forward packet with MAC header to NET layer for calculating
         * end-to-end latency and will reduce it later */
        NETSTACK_MAC.input();
      } else {//wrong message??
    	PRINTFL("slot %d: overheard msg from %d! \n", slot, hdr->sender.u8[0]);
    	if(we_are_learning == STILL_LEARNING){
    		update_distribution_table(slot - 1, &linkaddr_null, 0);
    		exploitation(slot - 1);
    	}
      }
    }else{
      NETSTACK_MAC.input();
      bc_recv ++;
#if SEND_BEACON_IN_BC_SLOT
    	  /* In order to improve the timesynch module for more frequently timestamp updated,
    	   * we wait for a beacon after receiving a broadcast message  */
    	  int len = 0;
    	  struct beacon beacon;
    	  t0 = RTIMER_NOW();
    	  on();
    	  while (RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + BEACON_WAIT)){
    		len = NETSTACK_RADIO.read(&beacon, sizeof(struct beacon));
    		if (len > 0){
    			off();
    			//printf("got beacon after bc\n");
    			timesynch_incoming_packet(beacon.send_time, last_arrival(), beacon.authority);
    			break;
    		}
    	  }
#endif
    	  /*printf("bc-from\t%d\tLQI\t%d\tseq\t%d\trecv\t%d\n", hdr->sender.u8[0], packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY),
    			  hdr->pkt_id, bc_recv);*/
    }
    break;
  }
  case TYPE_PREDATA: {
	listen_for_message = 1;
    break;
  }
  default: { /* nop */
    /* received an ACK while not listening for one ? */
  }
  }
  //printf("message received type %u \n",u);
} /*else { //something wrong with packet? maybe error when decoding

}*/
}
/*---------------------------------------------------------------------------*/
static u16_t bc_sq_no = 0;
static void
send_packet(mac_callback_t sent_callback, void *ptr)
{
  //PRINTFL("slot %d: NET send\n", SLOT_INDEX(RTIMER_NOW()));
//disable broadcast message during learning
/*if(we_are_learning == STILL_LEARNING && linkaddr_cmp(&linkaddr_null, packetbuf_addr(PACKETBUF_ADDR_RECEIVER))){
	return;
} else {*/
  struct queue_item* q = memb_alloc(&queue_memb);
  if (q != NULL){
    if (packetbuf_hdralloc(sizeof(struct much_hdr))){
      /* construct header here then copy, since packetbuf_hdr may not be aligned */
      struct much_hdr hdr;
      hdr.type.ptype = TYPE_DATA;
      hdr.type.seq = 0;
      hdr.mac_latency = 0;
      if(linkaddr_cmp(&linkaddr_null, packetbuf_addr(PACKETBUF_ADDR_RECEIVER))){
    	hdr.pkt_id = bc_sq_no ++;
      }
      linkaddr_copy(&(hdr.sender), &linkaddr_node_addr);
      linkaddr_copy(&(hdr.receiver), packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
      memcpy(packetbuf_hdrptr(),&hdr,sizeof(struct much_hdr));
      packetbuf_compact();
      q->packet = queuebuf_new_from_packetbuf();
      if (q->packet != NULL){
        q->txmits = 0;
        q->sent = 0;
        q->latency = 0;
        //q->epkt_id = packetbuf_attr(PACKETBUF_ATTR_EPACKET_ID);
        linkaddr_copy(&(q->ereceiver),packetbuf_addr(PACKETBUF_ADDR_ERECEIVER));
        linkaddr_copy(&(q->receiver),packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
#if WITH_REXMIT
        //no retransmission while we are learning
        if(we_are_learning == STILL_LEARNING){
    	  q->max_txmits = 1;
        } else{
        	q->max_txmits = 1;
    	 /* if (packetbuf_attr(PACKETBUF_ATTR_MAX_REXMIT)){
    		  q->max_txmits = packetbuf_attr(PACKETBUF_ATTR_MAX_REXMIT);
    	  } else {
    		  q->max_txmits = DEFAULT_MAX_TXMITS;
    	  }*/
        }
#else
        q->max_txmits = 1;
#endif
        q->sent_callback = sent_callback;
        q->sent_callback_ptr = ptr;
        if (linkaddr_cmp(&linkaddr_null, packetbuf_addr(PACKETBUF_ADDR_RECEIVER))){
          if (pending_bc == NULL){
            pending_bc = q;
          }
        }
        return;
      }
    }
  }
  /* if we didn't return, there was an error */
  if (q->packet != NULL){
    queuebuf_free(q->packet);
  }
  if (q != NULL){
    memb_free(&queue_memb, q);
  }
  PRINTF("error: packet dropped\n");
  mac_call_sent_callback(sent_callback, ptr, MAC_TX_ERR, 0);
//}
}
/*---------------------------------------------------------------------------*/
static int
turn_on(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
turn_off(int keep_radio_on)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static unsigned short
channel_check_interval(void)
{
  return CLOCK_SECOND * SLOT_TIME / RTIMER_SECOND;
}
/*---------------------------------------------------------------------------
static u8_t last_heartbeat;
static struct ctimer ht;
static void
heartbeat_listener(void* ptr)
{
  if (last_heartbeat == heartbeat){
    printf("Severe Error: node duty cycle stopped. Rebooting...\n");
    watchdog_reboot();
  } else {
    last_heartbeat = heartbeat;
    ctimer_set(&ht, CLOCK_SECOND * 2, heartbeat_listener, NULL);
  }
}
/*---------------------------------------------------------------------------*/
static void
al_mmac_init(void)
{
  we_are_learning = NOT_YET_LEARNING;
  memb_init(&queue_memb);
  process_start(&dutycycle_process, NULL);
  /*
  last_heartbeat = heartbeat;
  ctimer_set(&ht, CLOCK_SECOND * 2, heartbeat_listener, NULL);*/
}
/*---------------------------------------------------------------------------*/
const struct rdc_driver al_mmac_driver =
    { "AL_MMAC",
        al_mmac_init,
        send_packet,
        input_packet,
        turn_on,
        turn_off,
        channel_check_interval
    };
/*---------------------------------------------------------------------------*/
/** @} */
