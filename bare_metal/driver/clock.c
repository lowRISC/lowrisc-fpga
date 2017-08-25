/*
 * Copyright (c) 2016, Dr. Ralf Schlatterbeck Open Source Consulting
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
 *  \brief This module contains Lowrisc-specific code to implement
 *  the Contiki core clock functions.
 *  
 *  \author Ralf Schlatterbeck <rsc@runtux.com>.
 *
 */
/** \addtogroup lowrisc
 * @{
 */
/**
 *  \defgroup lowriscclock Lowrisc clock implementation
 * @{
 */
/**
 *  \file
 *  This file contains Lowrisc-specific code to implement the Contiki
 *  core clock functions.
 *
 */
/**
 * These routines define the Lowrisc-specific calls declared in
 * /core/sys/clock.h  CLOCK_SECOND is the number of ticks per second.
 * It is defined through CONF_CLOCK_SECOND in the contiki-conf.h for
 * each platform.
 * The usual AVR defaults are 128 or 125 ticks per second, counting a
 * prescaled CPU clock using the 8 bit timer0. We use the same in the
 * timer interrupt: 1/128 second ticks, this can be changed by modifying
 * CLOCK_TIMER_PERIOD below.
 * 
 * clock_time_t is usually declared by the platform as an unsigned 16
 * bit data type, thus intervals up to 512 or 524 seconds can be
 * measured with ~8 millisecond precision.
 * For longer intervals the 32 bit clock_seconds() is available.
 * We directly use the 64-bit cycle counter provided by the CPU.
 */
#include "sys/clock.h"
#include "sys/etimer.h"

#include <stdio.h>

/*---------------------------------------------------------------------------*/
/**
 * Return the tick counter. We use the full 64bit counter which makes
 * computation of seconds etc. easier later.
 */
    clock_time_t
clock_time(void)
{
  static clock_time_t temps;
  ++temps;
  return temps;
}
