/*
 * Copyright (c) 2014, Swedish Institute of Computer Science.
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
 */
/**
 * \file
 *         Example file using RPL for a any-to-any routing: the root and all
 *         nodes with an even node-id send a ping periodically to another node
 *         (which also must be root or have even node-id). Upon receiving a ping,
 *         nodes answer with a poing.
 *         Can be deployed in the Indriya or Twist testbeds.
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 *         Beshr Al Nahas <beshr@chalmers.se>
 */

#include "contiki-conf.h"
#include "net/netstack.h"
#include "net/mac/tsch/tsch-schedule.h"
#include "net/ipv6/uip-ds6-nbr.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/mac/tsch/tsch.h"
#include "net/ip/uip-debug.h"
#include "lib/random.h"
#include "deployment.h"
#include "simple-udp.h"
#include <stdio.h>
#include <string.h>

#if WITH_ORCHESTRA
#include "tools/orchestra.h"
#else
#include "static-schedules.h"
#endif

typedef struct {
  uint8_t node_id;
  uint16_t sf_handle;
  uint16_t size;
} node_sf_t;

typedef struct {
  uint16_t node_id;
  /* MAC address of neighbor */
  uint16_t nbr_id;
  /* Slotframe identifier */
  uint16_t slotframe_handle;
  /* Timeslot for this link */
  uint16_t timeslot;
  /* Channel offset for this link */
  uint16_t channel_offset;
  /* A bit string that defines
   * b0 = Transmit, b1 = Receive, b2 = Shared, b3 = Timekeeping, b4 = reserved */
  uint8_t link_options;
  /* Type of link. NORMAL = 0. ADVERTISING = 1, and indicates
     the link may be used to send an Enhanced beacon. */
  enum link_type link_type;
} link_t;

#define SEND_INTERVAL   (1 * CLOCK_SECOND)
#define UDP_PORT 1234

#define COORDINATOR_ID 1
#define DEST_ID COORDINATOR_ID
//#define DEST2_ID 4
//#define SRC_ID 3
#define WITH_PONG 0

static struct simple_udp_connection unicast_connection;
static uint16_t current_cnt = 0;
static uip_ipaddr_t llprefix;

/*---------------------------------------------------------------------------*/
PROCESS(unicast_sender_process, "No-RPL Unicast Application");
AUTOSTART_PROCESSES(&unicast_sender_process);
/*---------------------------------------------------------------------------*/
void app_send_to(uint16_t id, int ping, uint32_t seqno);
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *dataptr,
         uint16_t datalen)
{
  struct app_data data;
  appdata_copy((void *)&data, (void *)dataptr);
  if(data.ping) {
    LOGA((struct app_data *)dataptr, "App: received ping");
#if WITH_PONG
    app_send_to(UIP_HTONS(data.src), 0, data.seqno | 0x8000l);
#endif
  } else {
    LOGA((struct app_data *)dataptr, "App: received pong");
  }
}
/*---------------------------------------------------------------------------*/
void
app_send_to(uint16_t id, int ping, uint32_t seqno)
{
  struct app_data data;
  uip_ipaddr_t dest_ipaddr;

  data.magic = UIP_HTONL(LOG_MAGIC);
  data.seqno = UIP_HTONL(seqno);
  data.src = UIP_HTONS(node_id);
  data.dest = UIP_HTONS(id);
  data.hop = 0;
  data.ping = ping;

  if(ping) {
    LOGA(&data, "App: sending ping");
  } else {
    LOGA(&data, "App: sending pong");
  }
  set_ipaddr_from_id(&dest_ipaddr, id);
  /* Convert global address into link-local */
  //uip_lladdr_t dest_lladdr;
  //memcpy(&dest_ipaddr, &llprefix, 8);
  simple_udp_sendto(&unicast_connection, &data, sizeof(data), &dest_ipaddr);
}
#if !WITH_ORCHESTRA
void
app_make_schedule()
{
  node_sf_t n_sf[] = STATIC_SLOTFRAMES;
  link_t links[] = STATIC_LINKS;
  int i;
  /* add slotframes */
  for(i = 0; i < NUMBER_OF_SLOTFRAMES; i++) {
    node_sf_t nsf = n_sf[i];
    if(nsf.node_id == node_id) {
      tsch_schedule_add_slotframe(nsf.sf_handle, nsf.size);
    }
  }

  linkaddr_t addr;
  /* add links */
  for(i = 0; i < NUMBER_OF_LINKS; i++) {
    link_t l = links[i];
    if(l.node_id == node_id) {
      struct tsch_slotframe * sf = tsch_schedule_get_slotframe_from_handle(l.slotframe_handle);
      if(sf != NULL) {
        set_linkaddr_from_id(&addr, l.nbr_id);
        tsch_schedule_add_link(sf, l.link_options, l.link_type, &addr, l.timeslot, l.channel_offset);
        if((l.link_options & LINK_OPTION_TIME_KEEPING)
            && l.link_type != LINK_TYPE_ADVERTISING_ONLY
            && !linkaddr_cmp(&addr, &tsch_broadcast_address)
            && !linkaddr_cmp(&addr, &tsch_eb_address)) {
#if !WITH_RPL
          /* Setup default route */
          uip_ipaddr_t defrt_ipaddr;
          set_ipaddr_from_id(&defrt_ipaddr, l.nbr_id);
          /* Convert global address into link-local */
          memcpy(&defrt_ipaddr, &llprefix, 8);
          uip_ds6_nbr_add(&defrt_ipaddr, (const uip_lladdr_t *)&addr, 1, ADDR_MANUAL);
          uip_ds6_defrt_add(&defrt_ipaddr, 0 /* route timeout: never */);
#endif /* !WITH_RPL */
          /* setup as timesource */
          tsch_queue_update_time_source(&addr);
        }
      }
    }
  }
  tsch_queue_lock_time_source(1);

#if 0 /* should we create a minimum schedule too? */
  static struct tsch_slotframe *sf_min;
  /* Build 6TiSCH minimal schedule.
   * We pick a slotframe length of TSCH_SCHEDULE_DEFAULT_LENGTH */
  sf_min = tsch_schedule_add_slotframe(100, 101);
  /* Add a single Tx|Rx|Shared slot using broadcast address (i.e. usable for unicast and broadcast).
   * We set the link type to advertising, which is not compliant with 6TiSCH minimal schedule
   * but is required according to 802.15.4e if also used for EB transmission.
   * Timeslot: 0, channel offset: 0. */
  tsch_schedule_add_link(sf_min,
      LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED,
      LINK_TYPE_ADVERTISING_ONLY, &tsch_eb_address,
      0, 0);
#endif

  tsch_schedule_print();

  unsigned asn_val;
  for(asn_val = 0; asn_val < 20; asn_val++) {
    struct asn_t asn;
    ASN_INIT(asn, 0, asn_val);
    struct tsch_link *l = tsch_schedule_get_link_from_asn(&asn);
    if(l != NULL) {
      printf("asn %u: timeslot %u, channel offset %u (schedule handle %u)\n",
          asn_val, l->timeslot, l->channel_offset, l->slotframe_handle);
    } else {
      printf("asn %u: no link\n", asn_val);
    }
  }
}
#endif /* !WITH_ORCHESTRA */
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_sender_process, ev, data)
{
  static struct etimer periodic_timer;
  uip_ipaddr_t global_ipaddr;

  PROCESS_BEGIN();

  node_id_restore();
  if(node_id == COORDINATOR_ID) {
    tsch_is_coordinator = 1;
  }

  if(deployment_init(&global_ipaddr, NULL, -1)) {
    printf("App: %u starting\n", node_id);
  } else {
    printf("App: %u *not* starting\n", node_id);
    PROCESS_EXIT();
  }

  uip_ip6addr(&llprefix, 0xfe80, 0, 0, 0, 0, 0, 0, 0);
  simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);

#if WITH_ORCHESTRA
  orchestra_init();
#else
  app_make_schedule();
#endif

  if(node_id != DEST_ID) {
    etimer_set(&periodic_timer, SEND_INTERVAL);
    while(1) {
      app_send_to(DEST_ID, 1, ((uint32_t)node_id << 16) + current_cnt);
      current_cnt++;

      PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
      etimer_reset(&periodic_timer);
    }
  } else {
    while(1) {
      PROCESS_YIELD();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
