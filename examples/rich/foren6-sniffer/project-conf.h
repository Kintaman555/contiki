/*
 * Copyright (c) 2014, SICS Swedish ICT
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
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *         Project specific configuration defines for the android sniffer 
 *         example for the NXP jn5168 platform
 *
 * \author
 *         Beshr Al Nahas <beshr@sics.se>
 */

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#undef MICROMAC_RADIO_CONF_AUTOACK
#define MICROMAC_RADIO_CONF_AUTOACK 0

#define MICROMAC_CONF_ACCEPT_FCS_ERROR 1
#define RADIO_BUF_CONF_NUM 16 /* should be a power of two */

#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC      sniffer_rdc_driver

#define WITH_TSCH_LOG 0
#define WITH_RPL_LOG 0

/* The settings for the sniffer interface in foren6 are hard-coded to
 * No flow control
 * Baudrate: 115200bps
 * To change these settings you need to edit: foren6/capture/interface_snif.c
 * and then change the settings here.
 * PS: I have included an example under foren6-patch */
#define DEFAULT_FOREN6_SETTINGS 0

#undef UART_HW_FLOW_CTRL
#undef UART_SW_FLOW_CTRL
#undef UART_BAUD_RATE
#if DEFAULT_FOREN6_SETTINGS
#define UART_SW_FLOW_CTRL  0
#define UART_HW_FLOW_CTRL  0
#define UART_BAUD_RATE UART_RATE_115200
#else /* DEFAULT_FOREN6_SETTINGS */
#define UART_SW_FLOW_CTRL  0
#define UART_HW_FLOW_CTRL  1
#define UART_BAUD_RATE UART_RATE_1000000
#endif /* DEFAULT_FOREN6_SETTINGS */

#endif /* PROJECT_CONF_H_ */
