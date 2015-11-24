/*
 * Copyright (c) 2015, Swedish Institute of Computer Science.
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
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#include "contiki-conf.h"
#include "contiki-net.h"
#include "net/ip/uip.h"
#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/tsch-schedule.h"
#include "node-id.h"
#include "orchestra.h"
#include "toplogy.h"
#if CONTIKI_TARGET_SKY || CONTIKI_TARGET_Z1
#include "cc2420.h"
#endif
#include <string.h>
#include <stdio.h>

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

struct toplogy_link {
  linkaddr_t node;
  linkaddr_t parent;
};

/* List of ID<->MAC mapping used for different deployments */
static const struct toplogy_link topology[] = {
    {{{0x00,0x15,0x8d,0x00,0x00,0x52,0x69,0x9a}},{{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}}}, /* Coordinator entry */
    {{{0x00,0x15,0x8d,0x00,0x00,0x36,0x08,0x92}},{{0x00,0x15,0x8d,0x00,0x00,0x52,0x69,0x9a}}}, /* RICH196 -> Coordinator */
    {{{0x00,0x15,0x8d,0x00,0x00,0x36,0x01,0x80}},{{0x00,0x15,0x8d,0x00,0x00,0x36,0x08,0x92}}}, /* RICH001 -> RICH196 */
    {{{0x00,0x15,0x8d,0x00,0x00,0x57,0x5b,0x5d}},{{0x00,0x15,0x8d,0x00,0x00,0x36,0x08,0x92}}}, /* RICH197 -> RICH196 */
    {{{0x00,0x15,0x8d,0x00,0x00,0x36,0x08,0xb3}},{{0x00,0x15,0x8d,0x00,0x00,0x52,0x69,0x9a}}}, /* RICH198 -> Coordinator */
    {{{0x00,0x15,0x8d,0x00,0x00,0x36,0x08,0xb1}},{{0x00,0x15,0x8d,0x00,0x00,0x36,0x08,0xb3}}}, /* RICH193 -> RICH198 */
};

#define NODE_COUNT (sizeof(topology)/sizeof(struct toplogy_link))

/*---------------------------------------------------------------------------*/
unsigned
toplogy_orchestra_hash(const void *addr)
{
  int i;
  for(i=0; i<NODE_COUNT; i++) {
    if(linkaddr_cmp(addr, &topology[i].node)) {
      return i;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
const linkaddr_t *
toplogy_hardcoded_parent(const linkaddr_t *addr)
{
  int i;
  if(addr != NULL) {
    for(i=0; i<NODE_COUNT; i++) {
      if(linkaddr_cmp(addr, &topology[i].node)) {
        return &topology[i].parent;
      }
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
void *
toplogy_probing_func(void *vdag)
{
  /* Returns the next probing target. This implementation probes the current
   * preferred parent if we have not transmitted to it for RPL_PROBING_EXPIRATION_TIME.
   * Otherwise, it picks at random between:
   * (1) selecting the best parent (lowest rank) not updated for RPL_PROBING_EXPIRATION_TIME
   * (2) selecting the least recently updated parent
   */

  rpl_dag_t *dag = (rpl_dag_t *)vdag;
  rpl_parent_t *p;
  rpl_parent_t *probing_target = NULL;
  clock_time_t probing_target_age = 0;
  clock_time_t clock_now = clock_time();
  rpl_parent_t *hardcoded_parent = rpl_get_parent((uip_lladdr_t *)toplogy_hardcoded_parent(&linkaddr_node_addr));

  if(dag == NULL ||
      dag->instance == NULL ||
      dag->preferred_parent == NULL) {
    return NULL;
  }

  /* Our preferred parent needs probing */
  if(clock_now - dag->preferred_parent->last_tx_time > RPL_PROBING_EXPIRATION_TIME) {
    return dag->preferred_parent;
  }

  /* Our hardcoded preferred parent needs probing */
  if(hardcoded_parent != NULL
      && clock_now - hardcoded_parent->last_tx_time > RPL_PROBING_EXPIRATION_TIME) {
    return hardcoded_parent;
  }

  /* If we still do not have a probing target: pick the least recently updated parent */
  if(probing_target == NULL) {
    p = nbr_table_head(rpl_parents);
    while(p != NULL) {
      if(p->dag == dag) {
        if(probing_target == NULL
            || clock_now - p->last_tx_time > probing_target_age) {
          probing_target = p;
          probing_target_age = clock_now - p->last_tx_time;
        }
      }
      p = nbr_table_next(rpl_parents, p);
    }
  }

  return probing_target;
}
