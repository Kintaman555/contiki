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
 *         A simple offline scheduler for TSCH
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#include "contiki.h"

#if WITH_OFFLINE_SCHEDULE_SHARED || WITH_OFFLINE_SCHEDULE_DEDICATED

#include "net/packetbuf.h"
#include "net/rpl/rpl.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/tsch-private.h"
#include "net/mac/tsch/tsch-schedule.h"
#include "deployment.h"
#include "net/rime/rime.h"
#include <stdio.h>

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

#if WITH_OFFLINE_SCHEDULE_SHARED
static struct tsch_slotframe *sf_default;
static struct tsch_slotframe *sf_unicast;
// EB 397 co:0 so:Tx-id
// EB 397 co:0 so:Rx-TS-id
// Data-BC 17 co:1 so:Tx-Rx-Sh
// Data-UC 7 co:2 so:Rx-id%7
// Data-UC 7 co:2 so:TxSh-DestId%7
#define EB_SF_PERIOD 397
#define DEFAULT_SF_PERIOD 113
#define UNICAST_SF_PERIOD 23
#elif WITH_OFFLINE_SCHEDULE_DEDICATED
#define N_UNICAST_SF 2
static struct tsch_slotframe *sf_default;
static struct tsch_slotframe *sf_unicast[N_UNICAST_SF];
static int sf_unicast_len[N_UNICAST_SF] = {103,107};
#define EB_SF_PERIOD 397
#define DEFAULT_SF_PERIOD 61

#endif

/* Delete dedicated slots after 2 minutes */
#define DEDICATED_SLOT_LIFETIME TSCH_CLOCK_TO_SLOTS(2 * 60 * CLOCK_SECOND)

#if WITH_OFFLINE_SCHEDULE_DEDICATED
/*---------------------------------------------------------------------------*/
static void
offline_scheduler_delete_old_links(void)
{
  int i;
  for(i=0; i<N_UNICAST_SF; i++) {
    struct tsch_link *l = list_head(sf_unicast[i]->links_list);
    struct tsch_link *prev = NULL;
    /* Loop over all links and remove old ones. */
    while(l != NULL) {
      if(ASN_DIFF(current_asn, l->creation_asn) > DEDICATED_SLOT_LIFETIME) {
        uint16_t link_node_id = node_id_from_linkaddr(&l->addr);
        uint16_t link_node_index = get_node_index_from_id(link_node_id);
        PRINTF("Scheduler: removing link %u-%u from/to %u (index %u)\n",
            l->link_options, l->link_type, link_node_id, link_node_index);
        tsch_schedule_remove_link(sf_unicast[i], l);
        l = prev;
      }
      prev = l;
      l = list_item_next(l);
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
offline_scheduler_packet_received(void)
{
  if(packetbuf_attr(PACKETBUF_ATTR_PROTO) == UIP_PROTO_ICMP6) {
    /* Filter out ICMP */
    return;
  }

  int i;
  uint16_t dest_id = node_id_from_linkaddr(packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
  if(dest_id != 0) { /* Not a broadcast */
    uint16_t src_id = node_id_from_linkaddr(packetbuf_addr(PACKETBUF_ADDR_SENDER));
    uint16_t src_index = get_node_index_from_id(src_id);
    for(i=0; i<N_UNICAST_SF; i++) {
      /* Successful unicast Rx
       * We schedule a Rx link to listen to the source's dedicated slot,
       * in all unicast SFs */
      struct tsch_link *l = tsch_schedule_get_link_from_timeslot(sf_unicast[i], src_index);
      if(l == NULL) {
        PRINTF("Scheduler: adding Rx link for %u (index %u)\n", src_id, src_index);
        tsch_schedule_add_link(sf_unicast[i],
            LINK_OPTION_RX,
            LINK_TYPE_NORMAL, packetbuf_addr(PACKETBUF_ADDR_SENDER),
            src_index, 2+i);
      } else {
        PRINTF("Scheduler: updating Rx link for %u (index %u)\n", src_id, src_index);
        l->creation_asn = current_asn;
      }
    }
  }
  offline_scheduler_delete_old_links();
}
/*---------------------------------------------------------------------------*/
static void
offline_scheduler_packet_sent(int mac_status)
{
  if(packetbuf_attr(PACKETBUF_ATTR_PROTO) == UIP_PROTO_ICMP6) {
    /* Filter out ICMP */
    return;
  }

  int i;
  uint16_t dest_id = node_id_from_linkaddr(packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
  uint16_t dest_index = get_node_index_from_id(dest_id);
  if(dest_index != 0xffff && mac_status == MAC_TX_OK) {
    for(i=0; i<N_UNICAST_SF; i++) {
      /* Successful unicast Tx
       * We schedule a Tx link to this neighbor, in all unicast SFs */
      struct tsch_link *l = tsch_schedule_get_link_from_timeslot(sf_unicast[i], node_index);
      if(l == NULL || !linkaddr_cmp(&l->addr, packetbuf_addr(PACKETBUF_ADDR_RECEIVER))) {
        PRINTF("Scheduler: adding Tx link to %u (index %u)\n", dest_id, dest_index);
        tsch_schedule_add_link(sf_unicast[i],
            LINK_OPTION_TX,
            LINK_TYPE_NORMAL, packetbuf_addr(PACKETBUF_ADDR_RECEIVER),
            node_index, 2+i);
      } else {
        PRINTF("Scheduler: updating Tx link to %u (index %u)\n", dest_id, dest_index);
        l->creation_asn = current_asn;
      }
    }
  }
  offline_scheduler_delete_old_links();
}

RIME_SNIFFER(offline_scheduler_sniffer, offline_scheduler_packet_received, offline_scheduler_packet_sent);

void
offline_scheduler_init_dedicated()
{
  int i;

  sf_eb = tsch_schedule_add_slotframe(0, EB_SF_PERIOD);
  /* EB link: every neighbor uses its own to avoid contention */
  tsch_schedule_add_link(sf_eb,
      LINK_OPTION_TX,
      LINK_TYPE_ADVERTISING_ONLY, &tsch_broadcast_address,
      node_index, 0);

  /* Default slotframe: for broadcast or unicast to neighbors we
   * do not have a link to. */
  sf_default = tsch_schedule_add_slotframe(N_UNICAST_SF+1, DEFAULT_SF_PERIOD);
  tsch_schedule_add_link(sf_default,
      LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED,
      LINK_TYPE_NORMAL, &tsch_broadcast_address,
      0, 1);

  /* Slotframes for unicast */
  for(i=0; i<N_UNICAST_SF; i++) {
    sf_unicast[i] = tsch_schedule_add_slotframe(i+1, sf_unicast_len[i]);
  }

  rime_sniffer_add(&offline_scheduler_sniffer);
}
#endif /* WITH_OFFLINE_SCHEDULE_DEDICATED */

void
offline_scheduler_callback_new_time_source(struct tsch_neighbor *old, struct tsch_neighbor *new)
{
  uint16_t old_id = node_id_from_linkaddr(&old->addr);
  uint16_t old_index = get_node_index_from_id(old_id);
  uint16_t new_id = node_id_from_linkaddr(&new->addr);
  uint16_t new_index = get_node_index_from_id(new_id);

  if(new_index == old_index) {
    return;
  }

  if(old_index != 0xffff) {
    PRINTF("Scheduler: removing rx link for %u (%u) EB\n", old_id, old_index);
    /* Stop listening to the old time source's EBs */
    tsch_schedule_remove_link_from_timeslot(sf_eb, old_index);
#if WITH_OFFLINE_SCHEDULE_SHARED && UNICAST_SF_PERIOD > 0
    /* Shared-slot scheduler: remove unicast Tx link to the old time source */
    if(old_index % UNICAST_SF_PERIOD == node_index % UNICAST_SF_PERIOD) {
      /* This same link is also our unicast Rx link! Instead of removing it, we update it */
      PRINTF("Scheduler: removing tx option to link for %u (%u) unicast\n", old_id, old_index);
      tsch_schedule_add_link(sf_unicast,
          LINK_OPTION_RX,
          LINK_TYPE_NORMAL, &tsch_broadcast_address,
          node_index % UNICAST_SF_PERIOD, 2);
    } else {
      PRINTF("Scheduler: removing tx link for %u (%u) unicast\n", old_id, old_index);
      tsch_schedule_remove_link_from_timeslot(sf_unicast, old_index % UNICAST_SF_PERIOD);
    }
#endif
  }
  if(new_index != 0xffff) {
    /* Listen to the time source's EBs */
    PRINTF("Scheduler: adding rx link for %u (%u) EB\n", new_id, new_index);
    tsch_schedule_add_link(sf_eb,
        LINK_OPTION_RX,
        LINK_TYPE_ADVERTISING_ONLY, NULL,
        new_index, 0);

#if WITH_OFFLINE_SCHEDULE_SHARED && UNICAST_SF_PERIOD > 0
    /* Shared-slot scheduler: schedule a shared Tx link to the new time source */
    PRINTF("Scheduler: adding tx (rx=%d) link for %u (%u) unicast\n",
        new_index % UNICAST_SF_PERIOD == node_index % UNICAST_SF_PERIOD,
        new_id, new_index);
    tsch_schedule_add_link(sf_unicast,
        LINK_OPTION_TX | LINK_OPTION_SHARED
        /* If the source's timeslot and ours are the same, we must not only Tx but also Rx */
        | ((new_index % UNICAST_SF_PERIOD == node_index % UNICAST_SF_PERIOD) ? LINK_OPTION_RX : 0),
        LINK_TYPE_NORMAL, &new->addr,
        new_index % UNICAST_SF_PERIOD, 2);
#endif
  }
}

void
offline_scheduler_init_shared()
{
  sf_eb = tsch_schedule_add_slotframe(0, EB_SF_PERIOD);
  /* EB link: every neighbor uses its own to avoid contention */
  tsch_schedule_add_link(sf_eb,
      LINK_OPTION_TX,
      LINK_TYPE_ADVERTISING_ONLY, &tsch_broadcast_address,
      node_index, 0);

  /* Default slotframe: for broadcast or unicast to neighbors we
   * do not have a link to */
  sf_default = tsch_schedule_add_slotframe(2, DEFAULT_SF_PERIOD);
  tsch_schedule_add_link(sf_default,
      LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED,
      LINK_TYPE_NORMAL, &tsch_broadcast_address,
      0, 1);

#if UNICAST_SF_PERIOD > 0
  /* Slotframe for unicast rx */
  sf_unicast = tsch_schedule_add_slotframe(1, UNICAST_SF_PERIOD);
  /* Rx link, dedicated to us */
  /* Tx links are added from tsch_callback_new_time_source */
  tsch_schedule_add_link(sf_unicast,
      LINK_OPTION_RX,
      LINK_TYPE_NORMAL, &tsch_broadcast_address,
      node_index % UNICAST_SF_PERIOD, 2);
#endif
}

#endif /* WITH_OFFLINE_SCHEDULE_SHARED || WITH_OFFLINE_SCHEDULE_DEDICATED */
