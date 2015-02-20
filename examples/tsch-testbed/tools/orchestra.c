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
 *         Orchestra
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#include "contiki.h"

#include "net/packetbuf.h"
#include "net/rpl/rpl.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/tsch-private.h"
#include "net/mac/tsch/tsch-schedule.h"
#include "deployment.h"
#include "net/rime/rime.h"
#include "tools/orchestra.h"
#include <stdio.h>

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

#if ORCHESTRA_WITH_EBSF
static struct tsch_slotframe *sf_eb;
#endif
#if ORCHESTRA_WITH_COMMON_SHARED
static struct tsch_slotframe *sf_common;
#endif
#if ORCHESTRA_WITH_RBUNICAST
static struct tsch_slotframe *sf_rb;
#endif
#if ORCHESTRA_WITH_SBUNICAST
static struct tsch_slotframe *sf_sb;
/* Delete dedicated slots after 2 minutes */
#define DEDICATED_SLOT_LIFETIME TSCH_CLOCK_TO_SLOTS(2 * 60 * CLOCK_SECOND)
/* A net-layer sniffer for packets sent and received */
static void orchestra_packet_received(void);
static void orchestra_packet_sent(int mac_status);
RIME_SNIFFER(orhcestra_sniffer, orchestra_packet_received, orchestra_packet_sent);
#endif

#if ORCHESTRA_WITH_SBUNICAST
/*---------------------------------------------------------------------------*/
static void
orchestra_delete_old_links(void)
{
  struct tsch_link *l = list_head(sf_sb->links_list);
  struct tsch_link *prev = NULL;
  /* Loop over all links and remove old ones. */
  while(l != NULL) {
    if(ASN_DIFF(current_asn, l->creation_asn) > DEDICATED_SLOT_LIFETIME) {
      uint16_t link_node_id = node_id_from_linkaddr(&l->addr);
      uint16_t link_node_index = get_node_index_from_id(link_node_id);
      PRINTF("Orchestra: removing link %u-%u from/to %u (index %u)\n",
          l->link_options, l->link_type, link_node_id, link_node_index);
      tsch_schedule_remove_link(sf_sb, l);
      l = prev;
    }
    prev = l;
    l = list_item_next(l);
  }
}
/*---------------------------------------------------------------------------*/
static void
orchestra_packet_received(void)
{
  if(packetbuf_attr(PACKETBUF_ATTR_PROTO) == UIP_PROTO_ICMP6) {
    /* Filter out ICMP */
    return;
  }

  uint16_t dest_id = node_id_from_linkaddr(packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
  if(dest_id != 0) { /* Not a broadcast */
    uint16_t src_id = node_id_from_linkaddr(packetbuf_addr(PACKETBUF_ADDR_SENDER));
    uint16_t src_index = get_node_index_from_id(src_id);
    /* Successful unicast Rx
     * We schedule a Rx link to listen to the source's dedicated slot,
     * in all unicast SFs */
    struct tsch_link *l = tsch_schedule_get_link_from_timeslot(sf_sb, src_index);
    if(l == NULL) {
      PRINTF("Orchestra: adding Rx link for %u (index %u)\n", src_id, src_index);
      tsch_schedule_add_link(sf_sb,
          LINK_OPTION_RX,
          LINK_TYPE_NORMAL, packetbuf_addr(PACKETBUF_ADDR_SENDER),
          src_index, 2);
    } else {
      PRINTF("Orchestra: updating Rx link for %u (index %u)\n", src_id, src_index);
      l->creation_asn = current_asn;
    }
  }
  orchestra_delete_old_links();
}
/*---------------------------------------------------------------------------*/
static void
orchestra_packet_sent(int mac_status)
{
  if(packetbuf_attr(PACKETBUF_ATTR_PROTO) == UIP_PROTO_ICMP6) {
    /* Filter out ICMP */
    return;
  }

  uint16_t dest_id = node_id_from_linkaddr(packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
  uint16_t dest_index = get_node_index_from_id(dest_id);
  if(dest_index != 0xffff && mac_status == MAC_TX_OK) {
    /* Successful unicast Tx
     * We schedule a Tx link to this neighbor, in all unicast SFs */
    struct tsch_link *l = tsch_schedule_get_link_from_timeslot(sf_sb, node_index);
    if(l == NULL || !linkaddr_cmp(&l->addr, packetbuf_addr(PACKETBUF_ADDR_RECEIVER))) {
      PRINTF("Orchestra: adding Tx link to %u (index %u)\n", dest_id, dest_index);
      tsch_schedule_add_link(sf_sb,
          LINK_OPTION_TX | (ORCHESTRA_SBUNICAST_SHARED ? LINK_OPTION_SHARED : 0),
          LINK_TYPE_NORMAL, packetbuf_addr(PACKETBUF_ADDR_RECEIVER),
          node_index, 2);
    } else {
      PRINTF("Orchestra: updating Tx link to %u (index %u)\n", dest_id, dest_index);
      l->creation_asn = current_asn;
    }
  }
  orchestra_delete_old_links();
}
#endif /* ORCHESTRA_WITH_SBUNICAST */

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

#if ORCHESTRA_WITH_EBSF
  if(old_index != 0xffff) {
    PRINTF("Orchestra: removing rx link for %u (%u) EB\n", old_id, old_index);
    /* Stop listening to the old time source's EBs */
    tsch_schedule_remove_link_from_timeslot(sf_eb, old_index);
  }
  if(new_index != 0xffff) {
    /* Listen to the time source's EBs */
    PRINTF("Orchestra: adding rx link for %u (%u) EB\n", new_id, new_index);
    tsch_schedule_add_link(sf_eb,
        LINK_OPTION_RX,
        LINK_TYPE_ADVERTISING_ONLY, NULL,
        new_index, 0);
  }
#endif /* ORCHESTRA_WITH_EBSF */

#if ORCHESTRA_WITH_RBUNICAST
  if(old_index != 0xffff) {
    /* Shared-slot: remove unicast Tx link to the old time source */
    if(old_index % ORCHESTRA_RBUNICAST_PERIOD == node_index % ORCHESTRA_RBUNICAST_PERIOD) {
      /* This same link is also our unicast Rx link! Instead of removing it, we update it */
      PRINTF("Orchestra: removing tx option to link for %u (%u) unicast\n", old_id, old_index);
      tsch_schedule_add_link(sf_rb,
          LINK_OPTION_RX,
          LINK_TYPE_NORMAL, &tsch_broadcast_address,
          node_index % ORCHESTRA_RBUNICAST_PERIOD, 2);
    } else {
      PRINTF("Orchestra: removing tx link for %u (%u) unicast\n", old_id, old_index);
      tsch_schedule_remove_link_from_timeslot(sf_rb, old_index % ORCHESTRA_RBUNICAST_PERIOD);
    }
  }

  if(new_index != 0xffff) {
    /* Shared-slot: schedule a shared Tx link to the new time source */
    PRINTF("Orchestra: adding tx (rx=%d) link for %u (%u) unicast\n",
        new_index % ORCHESTRA_RBUNICAST_PERIOD == node_index % ORCHESTRA_RBUNICAST_PERIOD,
        new_id, new_index);
    tsch_schedule_add_link(sf_rb,
        LINK_OPTION_TX | LINK_OPTION_SHARED
        /* If the source's timeslot and ours are the same, we must not only Tx but also Rx */
        | ((new_index % ORCHESTRA_RBUNICAST_PERIOD == node_index % ORCHESTRA_RBUNICAST_PERIOD) ? LINK_OPTION_RX : 0),
        LINK_TYPE_NORMAL, &new->addr,
        new_index % ORCHESTRA_RBUNICAST_PERIOD, 2);
  }
#endif /* ORCHESTRA_WITH_RBUNICAST */
}

void
orchestra_init()
{
#if ORCHESTRA_WITH_EBSF
  sf_eb = tsch_schedule_add_slotframe(0, ORCHESTRA_EBSF_PERIOD);
  /* EB link: every neighbor uses its own to avoid contention */
  tsch_schedule_add_link(sf_eb,
      LINK_OPTION_TX,
      LINK_TYPE_ADVERTISING_ONLY, &tsch_broadcast_address,
      node_index, 0);
#endif

#if ORCHESTRA_WITH_RBUNICAST
  /* Receiver-based slotframe for unicast */
  sf_rb = tsch_schedule_add_slotframe(1, ORCHESTRA_RBUNICAST_PERIOD);
  /* Rx link, dedicated to us */
  /* Tx links are added from tsch_callback_new_time_source */
  tsch_schedule_add_link(sf_rb,
      LINK_OPTION_RX,
      LINK_TYPE_NORMAL, &tsch_broadcast_address,
      node_index % ORCHESTRA_RBUNICAST_PERIOD, 2);
#endif

#if ORCHESTRA_WITH_SBUNICAST
  /* Sender-based slotframe for unicast */
  sf_sb = tsch_schedule_add_slotframe(1, ORCHESTRA_SBUNICAST_PERIOD);
  /* Rx links (with lease time) will be added upon receiving unicast */
  /* Tx links (with lease time) will be added upon transmitting unicast (if ack received) */
  rime_sniffer_add(&orhcestra_sniffer);
#endif

#if ORCHESTRA_WITH_COMMON_SHARED
  /* Default slotframe: for broadcast or unicast to neighbors we
   * do not have a link to */
  sf_common = tsch_schedule_add_slotframe(2, ORCHESTRA_COMMON_SHARED_PERIOD);
  tsch_schedule_add_link(sf_common,
      LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED,
      ORCHESTRA_COMMON_SHARED_TYPE, &tsch_broadcast_address,
      0, 1);
#endif
}
