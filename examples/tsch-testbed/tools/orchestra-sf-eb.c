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

#ifndef ORCHESTRA_EBSF_PERIOD
#define ORCHESTRA_EBSF_PERIOD 397
#endif

static struct tsch_slotframe *sf_eb;

/*---------------------------------------------------------------------------*/
void
orchestra_sf_eb_new_time_source(struct tsch_neighbor *old, struct tsch_neighbor *new)
{
  uint16_t old_id = node_id_from_linkaddr(&old->addr);
  uint16_t old_index = get_node_index_from_id(old_id);
  uint16_t new_id = node_id_from_linkaddr(&new->addr);
  uint16_t new_index = get_node_index_from_id(new_id);

  if(new_index == old_index) {
    return;
  }

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
}

void
orchestra_sf_eb_init()
{
  sf_eb = tsch_schedule_add_slotframe(0, ORCHESTRA_EBSF_PERIOD);
  /* EB link: every neighbor uses its own to avoid contention */
  tsch_schedule_add_link(sf_eb,
      LINK_OPTION_TX,
      LINK_TYPE_ADVERTISING_ONLY, &tsch_broadcast_address,
      node_index, 0);
}
