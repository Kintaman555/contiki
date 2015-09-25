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
 *         RICH CoAP scheduler interface (header file)
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#define DAG_RESOURCE "rpl/dag"
#define DAG_PARENT_LABEL "parent"
#define DAG_CHILD_LABEL "child"

#define NEIGHBORS_RESOURCE "6top/nbrList"
#define NEIGHBORS_ASN "asn"
#define NEIGHBORS_TNA "tna"
#define NEIGHBORS_RSSI "rssi"
#define NEIGHBORS_LQI "lqi"

#define FRAME_RESOURCE "6top/slotFrame"
#define FRAME_ID_LABEL "frame"
#define FRAME_SLOTS_LABEL "slots"

#define LINK_RESOURCE "6top/cellList"
#define LINK_ID_LABEL "link"
#define LINK_SLOT_LABEL "slot"
#define LINK_CHANNEL_LABEL "channel"
#define LINK_OPTION_LABEL "option"
#define LINK_TYPE_LABEL "type"
#define LINK_TARGET_LABEL "target"
 

void rich_scheduler_interface_init();
