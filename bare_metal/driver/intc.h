// See LICENSE for license details.

#ifndef INTC_HEADER_H
#define INTC_HEADER_H

#include <stdint.h>
#include "dev_map.h"

/* Register offsets for the Xilinx AXI Interrupt controller Core */
#define INTC_ISR_OFFSET       0x00            /* Interrupt Status Register (ISR) */
#define INTC_IPR_OFFSET       0x04            /* Interrupt Pending Register (IPR) */
#define INTC_IER_OFFSET       0x08            /* Interrupt Enable Register (IER) */
#define INTC_IAR_OFFSET       0x0C            /* Interrupt Acknowledge Register (IAR) */
#define INTC_SIE_OFFSET       0x10            /* Set Interrupt Enables (SIE) */
#define INTC_CIE_OFFSET       0x14            /* Clear Interrupt Enables (CIE) */
#define INTC_IVR_OFFSET       0x18            /* Interrupt Vector Register (IVR) */
#define INTC_MER_OFFSET       0x1C            /* Master Enable Register (MER) */
#define INTC_IMR_OFFSET       0x20            /* Interrupt Mode Register (IMR) */
#define INTC_ILR_OFFSET       0x24            /* Interrupt Level Register (ILR) */

/* MDIO Address Register Bit Masks */
#define INTC_MER_IRQ_EN_MASK  0x00000001      /* Master IRQ enable mask */
#define INTC_MER_HW_EN_MASK   0x00000002      /* Hardware IRQ enable mask */

// INTC APIs
extern void intc_init();

#endif
