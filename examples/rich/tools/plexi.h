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

#ifndef __PLEXI_H__
#define __PLEXI_H___

#include "../common-conf.h"

#define PARENT_EVENT_RESOURCE(name, attributes, get_handler, post_handler, put_handler, delete_handler, event_handler) \
  resource_t name = { NULL, NULL, HAS_SUB_RESOURCES | IS_OBSERVABLE, attributes, get_handler, post_handler, put_handler, delete_handler, { .trigger = event_handler } }

#define PARENT_PERIODIC_RESOURCE(name, attributes, get_handler, post_handler, put_handler, delete_handler, period, periodic_handler) \
  periodic_resource_t periodic_##name; \
  resource_t name = { NULL, NULL, HAS_SUB_RESOURCES | IS_OBSERVABLE | IS_PERIODIC, attributes, get_handler, post_handler, put_handler, delete_handler, { .periodic = &periodic_##name } }; \
  periodic_resource_t periodic_##name = { NULL, &name, period, { { 0 } }, periodic_handler };

#define PLEXI_WITH_LINK_STATISTICS 0
#define PLEXI_WITH_QUEUE_STATISTICS 1
#define PLEXI_WITH_VICINITY_MONITOR 0
#define PLEXI_WITH_TRAFFIC_GENERATOR 0
#define PLEXI_NEIGHBOR_UPDATE_INTERVAL (10*CLOCK_SECOND)
#define PLEXI_LINK_UPDATE_INTERVAL (10*CLOCK_SECOND)


#define DAG_RESOURCE "rpl/dag"
#define DAG_PARENT_LABEL "parent"
#define DAG_CHILD_LABEL "child"

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
#define LINK_STATS_LABEL "stats"

#if PLEXI_WITH_LINK_STATISTICS
	#define PLEXI_DENSE_STATISTICS 1
	#define PLEXI_NOW_STATISTICS 2
	
	#ifndef PLEXI_STATISTICS_MODE
		#define PLEXI_STATISTICS_MODE PLEXI_DENSE_STATISTICS | PLEXI_SIMPLE_STATISTICS
	#endif
	
	#define TSCH_CALLBACK_REMOVE_LINK plexi_purge_link_statistics
	
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
	
	#define PLEXI_MAX_STATISTICS 7
	#define PLEXI_STATS_UPDATE_INTERVAL (10*CLOCK_SECOND)

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

	typedef enum { ENABLE, DISABLE } STATISTIC_METRIC_ACTIVITY;

	#if PLEXI_STATISTICS_MODE == PLEXI_DENSE_STATISTICS
	typedef uint16_t plexi_stats_value_t;
	typedef int16_t plexi_stats_value_st;
	#elif PLEXI_STATISTICS_MODE == PLEXI_DETAILED_STATISTICS
	typedef uint64_t plexi_stats_value_t;
	typedef int64_t plexi_stats_value_st;
	#endif

	typedef struct plexi_enhanced_stats_struct plexi_enhanced_stats;
	struct plexi_enhanced_stats_struct {
		plexi_enhanced_stats *next;
		linkaddr_t target;
		plexi_stats_value_t value;
	};

	typedef struct plexi_stats_struct plexi_stats;
	struct plexi_stats_struct {
		plexi_stats *next;
	#if PLEXI_STATISTICS_MODE & PLEXI_DENSE_STATISTICS == PLEXI_DENSE_STATISTICS
		uint16_t metainfo; /* enable:1lsb, metric:2-5lsb, id:6-10lsb, window:11-16lsb */
	#else
		uint16_t id;
		uint8_t enable;
		uint8_t metric;
		uint16_t window;
	#endif
		plexi_stats_value_t value;
	#if PLEXI_STATISTICS_MODE & PLEXI_NOW_STATISTICS == PLEXI_NOW_STATISTICS
		LIST_STRUCT(enhancement);
	#endif
	};

	#if PLEXI_WITH_VICINITY_MONITOR
		#define VICINITY_RESOURCE "mac/vicinity"
		#define VICINITY_AGE_LABEL "age"
		#define VICINITY_PHEROMONE_LABEL "pheromone"

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
	#endif

#endif


#if PLEXI_WITH_QUEUE_STATISTICS
	#define QUEUE_RESOURCE "6top/queueList"
	#define QUEUE_ID_LABEL "id"
	#define QUEUE_TXLEN_LABEL "txlen"
#endif


#if PLEXI_WITH_TRAFFIC_GENERATOR
	#define PLEXI_TRAFFIC_STEP (2*CLOCK_SECOND)
#endif

void plexi_init();

#endif
