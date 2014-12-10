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
 *         Override node_id_restore to node-id based on ds2411 ID
 *         in a testbed-specific manner
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#include "contiki-conf.h"
#include "deployment.h"

#if CONTIKI_TARGET_JN5168
#include <MMAC.h>
unsigned char node_mac[8];
#endif /* CONTIKI_TARGET_JN5168 */

unsigned short node_id = 0;
/*---------------------------------------------------------------------------*/
void
node_id_restore(void)
{
#if CONTIKI_TARGET_JN5168
  tuAddr psExtAddress;
  vMMAC_GetMacAddress(&psExtAddress.sExt);
  node_mac[7] = psExtAddress.sExt.u32L;
  node_mac[6] = psExtAddress.sExt.u32L >> (uint32_t)8;
  node_mac[5] = psExtAddress.sExt.u32L >> (uint32_t)16;
  node_mac[4] = psExtAddress.sExt.u32L >> (uint32_t)24;
  node_mac[3] = psExtAddress.sExt.u32H;
  node_mac[2] = psExtAddress.sExt.u32H >> (uint32_t)8;
  node_mac[1] = psExtAddress.sExt.u32H >> (uint32_t)16;
  node_mac[0] = psExtAddress.sExt.u32H >> (uint32_t)24;
#endif /* CONTIKI_TARGET_JN5168 */

  node_id = get_node_id();
}
/*---------------------------------------------------------------------------*/
void
node_id_burn(unsigned short id)
{
}
/*---------------------------------------------------------------------------*/
