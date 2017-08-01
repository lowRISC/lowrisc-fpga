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
 * \file PicoRV32 specific implementation of multithreading architecture
 *
 * \author Ralf Schlatterbeck <rsc@runtux.com>
 *
 */

#include <stdio.h>
#include "sys/mt.h"

/*--------------------------------------------------------------------------*/
void
mtarch_init(void)
{
  
}
/*--------------------------------------------------------------------------*/
void
mtarch_start(struct mtarch_thread *t,
	     void (*function)(void *), void *data)
{
  /* Initialize stack with number sequence (used for
   * measuring stack usage */
  uint8_t i;

  printf ("mtarch_start called\n");
  for(i = 0; i < MTARCH_STACKSIZE; ++i) {
    t->stack[i] = i;
  }

  /*
   * Push pointer to mt_exit and the thread onto our stack:
   * Caveats:
   *  - The stack is defined as an array of bytes, but pointers are 32 bit wide
   *  - Function pointers are 32-bit addresses
   */

  /* Initialize stack. This is done in reverse order ("pushing") the
   *  pre-allocated array */

  /* mt_exit function that is to be invoked if the thread dies
   * function is the thread handler. Invoked when RET is called in mtarch_exec
   */
  *((uint32_t *)(t->stack + MTARCH_STACKSIZE - 4)) = (uint32_t)mt_exit;
  *((uint32_t *)(t->stack + MTARCH_STACKSIZE - 8)) = (uint32_t)function;

  /* FIXME: Push args to function and saved registers, check calling
   * conventions
   */

  /* Initialize stack pointer: Space for 2 2-byte-addresses and 32 registers,
   * post-decrement POP / pre-increment PUSH scheme
   * FIXME: Need to calculate correct address here
   */
  t->sp = &t->stack[MTARCH_STACKSIZE - 1 - 4 - 32];
}

/*--------------------------------------------------------------------------*/
static struct mtarch_thread *running;

static void
sw(void)
{
  printf ("sw called\n");
  /* FIXME: Disable interrupts while we perform the context switch */
  /* Needs to be in separate asm statement, we don't want to be
   * interrupted while the C-Compiler-generated wrapper-code pushes
   * registers on the stack.
   */

  /*
   * Need to save ra, s0/fp, s1-s11, we make the C-compiler do it by
   * specifying these registers as clobber.
   * For now we leave MT threads alone -- the stack management is too
   * unstable in the currently-used gcc port, in our example for storing
   * 13 4-byte variables on the stack the compiler allocates 64 bytes on
   * the stack (52 would be ok, 64 is not even explained if the stack is
   * kept 8-byte aligned (maybe 16?)). Also the normal function wrapper
   * code is not called if we have a single asm statement in a function
   * (the normal wrapper code already saves ra, s0/fp on the stack).
   */

  /* jrrk */
}
/*--------------------------------------------------------------------------*/
void
mtarch_exec(struct mtarch_thread *t)
{
  printf ("mtarch_exec called\n");
  running = t;
  sw();
  running = NULL;
}

/*--------------------------------------------------------------------------*/
void
mtarch_remove(void)
{

}
/*--------------------------------------------------------------------------*/
void
mtarch_yield(void)
{
  sw();
}
/*--------------------------------------------------------------------------*/
void
mtarch_pstop(void)
{
  
}
/*--------------------------------------------------------------------------*/
void
mtarch_pstart(void)
{
  
}
/*--------------------------------------------------------------------------*/
void
mtarch_stop(struct mtarch_thread *t)
{
  
}
/*--------------------------------------------------------------------------*/
int
mtarch_stack_usage(struct mt_thread *t)
{
  uint8_t i;
  for(i = 0; i < MTARCH_STACKSIZE; ++i) {
    if(t->thread.stack[i] != i) {
      break;
    }
  }
  return MTARCH_STACKSIZE - i;
}
/*--------------------------------------------------------------------------*/
