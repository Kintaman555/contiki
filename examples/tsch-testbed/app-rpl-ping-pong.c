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
 *         Example file using RPL for a data collection.
 *         Can be deployed in the Indriya or Twist testbeds.
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#include "contiki-conf.h"
#include "net/netstack.h"
#include "net/mac/tsch/tsch-schedule.h"
#include "net/rpl/rpl-private.h"
#include "net/mac/tsch/tsch-schedule.h"
#include "net/ip/uip-debug.h"
#include "lib/random.h"
#include "net/mac/tsch/tsch-rpl.h"
#include "deployment.h"
#include "simple-udp.h"
#include "tools/orchestra.h"
#include <stdio.h>

#define SEND_INTERVAL   (2*CLOCK_SECOND)
#define WITH_PONG 1
#define UDP_PORT 1234

static struct simple_udp_connection unicast_connection;

/*---------------------------------------------------------------------------*/
PROCESS(unicast_sender_process, "Ping-pong Application");
AUTOSTART_PROCESSES(&unicast_sender_process);
/*---------------------------------------------------------------------------*/
int app_send_to(uint16_t id, uint32_t seqno, int ping);
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
#if WITH_PONG
    LOGA((struct app_data *)dataptr, "App: received ping");
    app_send_to(UIP_HTONS(data.src), data.seqno | 0x8000l, 0);
#else
    LOGA((struct app_data *)dataptr, "App: received");
#endif
  } else {
    LOGA((struct app_data *)dataptr, "App: received pong");
  }
}
/*---------------------------------------------------------------------------*/
int
can_send_to(uip_ipaddr_t *ipaddr) {
  return uip_ds6_is_addr_onlink(ipaddr)
      || uip_ds6_route_lookup(ipaddr)
      || uip_ds6_defrt_choose();
}
/*---------------------------------------------------------------------------*/
int
app_send_to(uint16_t id, uint32_t seqno, int ping)
{
  struct app_data data;
  uip_ipaddr_t dest_ipaddr;

  data.magic = UIP_HTONL(LOG_MAGIC);
  data.seqno = UIP_HTONL(seqno);
  data.src = UIP_HTONS(node_id);
  data.dest = UIP_HTONS(id);
  data.hop = 0;
  data.ping = ping;

  set_ipaddr_from_id(&dest_ipaddr, id);

  if(can_send_to(&dest_ipaddr)) {
#if WITH_PONG
    if(ping) {
      LOGA(&data, "App: sending ping");
    } else {
      LOGA(&data, "App: sending pong");
    }
#else
    LOGA(&data, "App: sending");
#endif
    simple_udp_sendto(&unicast_connection, &data, sizeof(data), &dest_ipaddr);
    return 1;
  } else {
    data.seqno = UIP_HTONL(seqno);
    LOGA(&data, "App: could not send");
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_sender_process, ev, data)
{
  static struct etimer periodic_timer;
  uip_ipaddr_t global_ipaddr;
  static unsigned int cnt;
  static unsigned int to_send_cnt;
  static uint32_t seqno;

  PROCESS_BEGIN();

  if(deployment_init(&global_ipaddr, NULL, ROOT_ID)) {
    //LOG("App: %u start\n", node_id);
  } else {
    PROCESS_EXIT();
  }
  simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);

#if WITH_TSCH
#if WITH_ORCHESTRA
  orchestra_init();
#else
  tsch_schedule_create_minimal();
#endif
#endif

  if(node_id == ROOT_ID) {
    etimer_set(&periodic_timer, SEND_INTERVAL);
    while(1) {
      if(default_instance != NULL && uip_ds6_route_num_routes() > MAX_NODES/2) {
        uint16_t target_id;
        uip_ipaddr_t target_ipaddr;
        do {
          target_id = get_random_node_id();
          set_ipaddr_from_id(&target_ipaddr, target_id);
        } while(target_id == node_id || !can_send_to(&target_ipaddr));
        seqno = ((uint32_t)node_id << 16) + cnt;
        app_send_to(target_id, seqno, 1);
        cnt++;

      } else {
        LOG("App: no DODAG or not enough routes (%u <= %u)\n", uip_ds6_route_num_routes(), MAX_NODES/2);
      }

      PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
      etimer_reset(&periodic_timer);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
