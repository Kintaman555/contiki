/*
 * Copyright (c) 2009, Vrije Universiteit Brussel.
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
 *         A multi-channel power saving MAC protocol
 * \author
 *         Joris Borms <joris.borms@vub.ac.be>
 */

#ifndef __AL_MMAC_H__
#define __AL_MMAC_H__

#include "sys/rtimer.h"
#include "net/mac/rdc.h"
#include "dev/radio.h"
#include "net/linkaddr.h"



struct type_hdr {
  u8_t ptype;
  u8_t seq;  /* used a sequence nbr for TYPE_DATA, ack/nack flag for TYPE_ACK */
};

/*enum mac_type {
  TYPE_PREDATA,
  TYPE_BEACON,
  TYPE_DATA,
  TYPE_ACK
};
struct beacon {
  struct type_hdr type;
  linkaddr_t sender;
  linkaddr_t receiver; //Huy added
  u16_t           authority;
  rtimer_clock_t   send_time;
};*/

struct much_hdr {
  struct type_hdr type;
  linkaddr_t  sender;
  linkaddr_t  receiver;
  u8_t mac_latency;
  u16_t pkt_id;
};

#define SLOTS_PER_FRAME 	15 /* should be power of 2 minus 1 */
#define SLOT_TIME 			(RTIMER_SECOND / 16) /* should be power of 2, max value = 64*/

void learning_start(void);
void update_from_net(struct collect_neighbor_list *list, u16_t rtmetric);
int learning_done(void);

extern const struct rdc_driver al_mmac_driver;


#endif /* __AL_MMAC_H__ */
