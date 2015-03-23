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

#if WITH_ORCHESTRA

#include "contiki.h"

#include "lib/memb.h"
#include "net/packetbuf.h"
#include "net/rpl/rpl.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/tsch-private.h"
#include "net/mac/tsch/tsch-schedule.h"
#include "deployment.h"
#include "net/rime/rime.h"
#include "tools/orchestra.h"
#include <stdio.h>

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

static struct tsch_slotframe *sf_rb;

#ifndef ORCHESTRA_RBUNICAST_PERIOD
#define ORCHESTRA_RBUNICAST_PERIOD 17
#endif

void
orchestra_sf_rb_unicast_new_time_source(struct tsch_neighbor *old, struct tsch_neighbor *new)
{
  uint16_t old_id = node_id_from_linkaddr(&old->addr);
  uint16_t old_index = get_node_index_from_id(old_id);
  uint16_t new_id = node_id_from_linkaddr(&new->addr);
  uint16_t new_index = get_node_index_from_id(new_id);

  if(new_index == old_index) {
    return;
  }

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
}

void
orchestra_sf_rb_unicast_init()
{
  /* Receiver-based slotframe for unicast */
  sf_rb = tsch_schedule_add_slotframe(2, ORCHESTRA_RBUNICAST_PERIOD);
  /* Rx link, dedicated to us */
  /* Tx links are added from tsch_callback_new_time_source */
  tsch_schedule_add_link(sf_rb,
      LINK_OPTION_RX,
      LINK_TYPE_NORMAL, &tsch_broadcast_address,
      node_index % ORCHESTRA_RBUNICAST_PERIOD, 2);
}
#endif /* WITH_ORCHESTRA */
