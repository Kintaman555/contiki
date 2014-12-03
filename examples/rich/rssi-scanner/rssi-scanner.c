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
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Scanning 2.4 GHz radio frequencies using jn5168 and prints
 *         the values
 * \author
 *         Joakim Eriksson, joakime@sics.se
 *
 *         ported to jn5168 by:
 *         Beshr Al Nahas, beshr@sics.se
 */

#include "contiki.h"
#include "Utilities/include/JPT.h"
#include <stdio.h>
#include <jendefs.h>
#include "contiki-conf.h"
#include "dev/watchdog.h"

/* valid frequency range: [2350 to 2550] */
#define MIN_FREQUENCY 2400
#define MAX_FREQUENCY 2485
#define NUM_SAMPLES 2
/*---------------------------------------------------------------------------*/
static void
do_rssi_basic(void)
{
  uint8_t channel;
  uint8_t detected_energy;
  int16_t rssi;
  uint32_t samples = NUM_SAMPLES;
  uint8_t i;
  printf("RSSI:");
  for(channel = 11; channel <= 26; ++channel) {
    /* we need 5 rssi values per channel to be compatible with the viewer */
    for(i = 0; i < 5; i++) {
      detected_energy = u8JPT_EnergyDetect(channel, samples);
      /* Energy level expressed in dBm. The range of possible output values is:
      –11 dBm to –98 dBm, with 2 dBm step accuracy */
      rssi = i16JPT_ConvertEnergyTodBm(detected_energy);
      /* An RSSI=0 corresponds to -100 dBm, an RSSI=100 corresponds to 0 dBm */
      printf("%d ", 100 + rssi);
    }
  }
  printf("\n");
}
/*---------------------------------------------------------------------------*/
static void
do_rssi_fine(void)
{
  uint32_t frequency;
  uint8_t detected_energy;
  int16_t rssi;
  uint32_t samples = NUM_SAMPLES;
  uint8_t i;
  printf("RSSI:");
//  Fc = 2405 + 5(k-11) MHz, k=11,12, ... , 26
  for(frequency = 2400; frequency <= 2485; frequency++) {
    /* valid frequencies: 2350 to 2550 */
    detected_energy = u8JPT_FineTuneEnergyDetect(frequency, samples);
    /* Energy level expressed in dBm. The range of possible output values is:
     –11 dBm to –98 dBm, with 2 dBm step accuracy */
    rssi = i16JPT_ConvertEnergyTodBm(detected_energy);
    /* An RSSI=0 corresponds to -100 dBm, an RSSI=100 corresponds to 0 dBm */
    printf("%d ", 100 + rssi);
  }
  printf("\n");
}
/*---------------------------------------------------------------------------*/
PROCESS(scanner_process, "RSSI Scanner");
AUTOSTART_PROCESSES(&scanner_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(scanner_process, ev, data)
{
  PROCESS_BEGIN();

  /* We use NullRadio so we control the radio directly with JPT library */
  uint32_t jpt_ver = u32JPT_Init();
  uint32_t radio_mode = 0;
  /* Get the Modes supported by this device */
  radio_mode = u32JPT_RadioModesAvailable();
  if(radio_mode & (1 << E_JPT_MODE_HIPOWER)) {
    /* High power mode supported, switching to it */
    radio_mode = E_JPT_MODE_HIPOWER;
  } else {
    /* Normal mode */
    radio_mode = E_JPT_MODE_LOPOWER;
  }
  /* Initialize the radio */
  int radio_init = bJPT_RadioInit(radio_mode);

  printf("Starting RSSI scanner. JPT version: %u, radio init: %d, radio mode: %s\n",
      jpt_ver, radio_init,
      (radio_mode == E_JPT_MODE_HIPOWER) ? "High power mode" : "Normal mode");

  while(1) {
    do_rssi_fine();
//    PROCESS_PAUSE();
    watchdog_periodic();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
