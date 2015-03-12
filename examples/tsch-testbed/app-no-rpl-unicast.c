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

#define SEND_INTERVAL   (5 * CLOCK_SECOND)
#define UDP_PORT 1234

#define COORDINATOR_ID 2
#define DEST_ID 2
#define DEST2_ID 4
#define SRC_ID 3
#define WITH_PONG 0

static struct simple_udp_connection unicast_connection;
static uint16_t current_cnt = 0;
static uip_ipaddr_t llprefix;

/* TODO: Disable RPL, and implement static routing.
 * Use the option:
 * CONTIKI_WITH_RPL=0 in Makefile
 * WITH_RPL 0 in project-conf
 * to add a default route use the function :
 * uip_ds6_defrt_t* uip_ds6_defrt_add(uip_ipaddr_t *ipaddr, unsigned long interval)
 *  */
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
  uip_lladdr_t dest_lladdr;

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
  memcpy(&dest_ipaddr, &llprefix, 8);
  set_linkaddr_from_id((linkaddr_t *)&dest_lladdr, id);
  uip_ds6_nbr_add(&dest_ipaddr, &dest_lladdr, 1, ADDR_MANUAL);
  simple_udp_sendto(&unicast_connection, &data, sizeof(data), &dest_ipaddr);
}
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
  tsch_schedule_create_minimal();
#endif

  /* Setup default route */
  uip_ipaddr_t defrt_ipaddr;
  set_ipaddr_from_id(&defrt_ipaddr, COORDINATOR_ID);
  uip_ds6_defrt_add(&defrt_ipaddr, 0 /* route timeout: never */);

  if(node_id == SRC_ID) {
    etimer_set(&periodic_timer, SEND_INTERVAL);
    while(1) {
      app_send_to(current_cnt % 3 == 0 ? DEST_ID : DEST2_ID , 1, ((uint32_t)node_id << 16) + current_cnt);
      //app_send_to(DEST_ID, 1, ((uint32_t)node_id << 16) + current_cnt);
      current_cnt++;

      PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
      etimer_reset(&periodic_timer);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
