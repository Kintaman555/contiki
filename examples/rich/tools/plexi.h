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

#include "../common-conf.h"

#define DAG_RESOURCE "rpl/dag"
#define DAG_PARENT_LABEL "parent"
#define DAG_CHILD_LABEL "child"

#define VICINITY_RESOURCE "mac/vicinity"
#define VICINITY_AGE_LABEL "age"
#define VICINITY_PHEROMONE_LABEL "pheromone"

#define NEIGHBORS_RESOURCE "6top/nbrList"
#define NEIGHBORS_ASN_LABEL "asn"
#define NEIGHBORS_TNA_LABEL "tna"

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
#define LINK_STATS_LABEL "stats"

#define STATS_RESOURCE "6top/stats"
#define STATS_METRIC_LABEL "metric"
#define STATS_ID_LABEL "id"
#define STATS_VALUE_LABEL "value"
#define STATS_WINDOW_LABEL "window"
#define STATS_ENABLE_LABEL "enable"
#define STATS_ETX_LABEL "etx"
#define STATS_RSSI_LABEL "rssi"
#define STATS_LQI_LABEL "lqi"
#define STATS_PDR_LABEL "pdr"

#define PLEXI_PHEROMONE_CHUNK 10
#define PLEXI_PHEROMONE_DECAY 3
#define PLEXI_PHEROMONE_WINDOW 10 * CLOCK_SECOND

#define PLEXI_MAX_PROXIMATES 15

typedef struct plexi_proximate_struct plexi_proximate;
struct plexi_proximate_struct {
	struct plexi_proximate_struct *next;
	linkaddr_t proximate;
	clock_time_t since;
	uint8_t pheromone;
};

#define PLEXI_MAX_STATISTICS 15

typedef enum {
	NONE = 0,
	//macCounterOctets = 1,
	//macRetryCount = 2,
	//macMultipleRetryCount = 3,
	//macTXFailCount = 4,
	//macTXSuccessCount = 5,
	//macFCSErrorCount = 6,
	//macSecurityFailure = 7,
	//macDuplicateFrameCount = 8,
	//macRXSuccessCount = 9,
	//macNACKcount = 10,
	PDR = 11,
	ETX = 12,
	RSSI = 13,
	LQI = 14,
	ASN = 15
} STATISTIC_METRIC;

typedef enum { ENABLE, DISABLE, ANY } STATISTIC_METRIC_ACTIVITY;

typedef struct plexi_stats_struct plexi_stats;
struct plexi_stats_struct {
	plexi_stats *next;
	uint16_t id;
	uint8_t enable;
	uint16_t window;
	uint8_t metric;
	uint16_t value;
};

void plexi_init();
