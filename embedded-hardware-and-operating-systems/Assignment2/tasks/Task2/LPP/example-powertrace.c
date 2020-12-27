/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
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
 *         Demonstrating the powertrace application with broadcasts
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"

#include "powertrace.h"

#include "dev/button-sensor.h"

#include "dev/leds.h"

#include <stdio.h>

#include "sys/energest.h"

#define V 3.6
#define I_CPU 0.0018
#define I_LPM 0.0000545
#define I_TRANSMIT 0.0195
#define I_RECEIVING 0.0218

/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "BROADCAST example");
AUTOSTART_PROCESSES(&example_broadcast_process);
/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  printf("broadcast message received from %d.%d: '%s'\n",
         from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_broadcast_process, ev, data)
{
  static struct etimer et;

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  /* Start powertracing, once every two seconds. */
  //powertrace_start(CLOCK_SECOND * 2);

  broadcast_open(&broadcast, 129, &broadcast_call);

  static uint32_t last_cpu, last_lpm, last_transmit, last_listen;
  uint32_t all_cpu, all_lpm, all_transmit, all_listen;
  uint32_t cpu, lpm, transmit, listen;

  // https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-Energest
  // contiki/apps/powertrace/powertrace.c
  energest_flush();

  while(1) {

    /* Delay 2-4 seconds */
    etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    packetbuf_copyfrom("Hello", 6);
    broadcast_send(&broadcast);
    printf("broadcast message sent\n");

    all_cpu = energest_type_time(ENERGEST_TYPE_CPU);
    all_lpm = energest_type_time(ENERGEST_TYPE_LPM);
    all_transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
    all_listen = energest_type_time(ENERGEST_TYPE_LISTEN);

    cpu = all_cpu - last_cpu;
    lpm = all_lpm - last_lpm;
    transmit = all_transmit - last_transmit;
    listen = all_listen - last_listen;

    last_cpu = energest_type_time(ENERGEST_TYPE_CPU);
    last_lpm = energest_type_time(ENERGEST_TYPE_LPM);
    last_transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
    last_listen = energest_type_time(ENERGEST_TYPE_LISTEN);

    double eCpu = (I_CPU * V) / CLOCK_SECOND;
    unsigned int eCpuI = eCpu * cpu * 1000;

    double eLpm = (I_LPM * V) / CLOCK_SECOND;
    unsigned int eLpmI = eLpm * lpm * 1000;

    double eTransmit = (I_TRANSMIT * V) / CLOCK_SECOND;
    unsigned int eTransmitI = eTransmit * transmit * 1000;

    double eReceiving = (I_RECEIVING * V) / CLOCK_SECOND;
    unsigned int eReceivingI = eReceiving * listen * 1000;

    printf("cpu=%d.%03u lpm=%d.%03u transmit=%d.%03u receiving=%d.%03u\n",
        eCpuI / 1000, eCpuI % 1000,
        eLpmI / 1000, eLpmI % 1000,
        eTransmitI / 1000, eTransmitI % 1000,
        eReceivingI / 1000, eReceivingI % 1000);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
