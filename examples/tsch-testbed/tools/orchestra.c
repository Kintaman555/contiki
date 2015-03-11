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
#if ORCHESTRA_WITH_EBSF
#include "tools/orchestra-sf-eb.h"
#endif
#if ORCHESTRA_WITH_COMMON_SHARED
#include "tools/orchestra-sf-common-shared.h"
#endif
#if ORCHESTRA_WITH_RBUNICAST
#include "tools/orchestra-sf-rb-unicast.h"
#endif
#if ORCHESTRA_WITH_SBUNICAST
#include "tools/orchestra-sf-sb-unicast.h"
#endif
#include <stdio.h>

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

void
orchestra_callback_new_time_source(struct tsch_neighbor *old, struct tsch_neighbor *new)
{
#if ORCHESTRA_WITH_EBSF
  orchestra_sf_eb_new_time_source(old, new);
#endif

#if ORCHESTRA_WITH_RBUNICAST
  orchestra_sf_rb_unicast_new_time_source(old, new);
#endif

#if ORCHESTRA_WITH_SB_RPL
  orchestra_sf_rb_rpl_new_time_source(old, new);
#endif
}

void
orchestra_init()
{
#if ORCHESTRA_WITH_EBSF
  orchestra_sf_eb_init();
#endif

#if ORCHESTRA_WITH_COMMON_SHARED
  orchestra_sf_common_shared_init();
#endif

#if ORCHESTRA_WITH_RBUNICAST
  orchestra_sf_rb_unicast_init();
#endif

#if ORCHESTRA_WITH_SBUNICAST
  orchestra_sf_sb_unicast_init();
#endif

#if ORCHESTRA_WITH_SB_RPL
  orchestra_sf_sb_rpl_unicast_init();
#endif
}
