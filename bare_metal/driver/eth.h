// See LICENSE for license details.

#ifndef ETH_HEADER_H
#define ETH_HEADER_H

#include <stdint.h>
#include "dev_map.h"

// Xilinx AXI_ETH 16550

#ifdef DEV_MAP__io_ext_eth__BASE
  #define ETH_BASE ((uint32_t)(DEV_MAP__io_ext_eth__BASE))
#else
  #define ETH_BASE 0
#endif

/* Register offsets for the EmacLite Core */
#define XEL_TXBUFF_OFFSET       0x0             /* Transmit Buffer */
#define XEL_MDIOADDR_OFFSET     0x07E4          /* MDIO Address Register */
#define XEL_MDIOWR_OFFSET       0x07E8          /* MDIO Write Data Register */
#define XEL_MDIORD_OFFSET       0x07EC          /* MDIO Read Data Register */
#define XEL_MDIOCTRL_OFFSET     0x07F0          /* MDIO Control Register */
#define XEL_GIER_OFFSET         0x07F8          /* GIE Register */
#define XEL_TSR_OFFSET          0x07FC          /* Tx status */
#define XEL_TPLR_OFFSET         0x07F4          /* Tx packet length */

#define XEL_RXBUFF_OFFSET       0x1000          /* Receive Buffer */
#define XEL_RPLR_OFFSET         0x100C          /* Rx packet length */
#define XEL_RSR_OFFSET          0x17FC          /* Rx status */

#define XEL_BUFFER_OFFSET       0x0800          /* Next Tx/Rx buffer's offset */

/* MDIO Address Register Bit Masks */
#define XEL_MDIOADDR_REGADR_MASK  0x0000001F    /* Register Address */
#define XEL_MDIOADDR_PHYADR_MASK  0x000003E0    /* PHY Address */
#define XEL_MDIOADDR_PHYADR_SHIFT 5
#define XEL_MDIOADDR_OP_MASK      0x00000400    /* RD/WR Operation */

/* MDIO Write Data Register Bit Masks */
#define XEL_MDIOWR_WRDATA_MASK    0x0000FFFF    /* Data to be Written */

/* MDIO Read Data Register Bit Masks */
#define XEL_MDIORD_RDDATA_MASK    0x0000FFFF    /* Data to be Read */

/* MDIO Control Register Bit Masks */
#define XEL_MDIOCTRL_MDIOSTS_MASK 0x00000001    /* MDIO Status Mask */
#define XEL_MDIOCTRL_MDIOEN_MASK  0x00000008    /* MDIO Enable */

/* Global Interrupt Enable Register (GIER) Bit Masks */
#define XEL_GIER_GIE_MASK       0x80000000      /* Global Enable */

/* Transmit Status Register (TSR) Bit Masks */
#define XEL_TSR_XMIT_BUSY_MASK   0x00000001     /* Tx complete */
#define XEL_TSR_PROGRAM_MASK     0x00000002     /* Program the MAC address */
#define XEL_TSR_XMIT_IE_MASK     0x00000008     /* Tx interrupt enable bit */
#define XEL_TSR_XMIT_ACTIVE_MASK 0x80000000     /* Buffer is active, SW bit
                                                 * only. This is not documented
                                                 * in the HW spec */

/* Define for programming the MAC address into the EmacLite */
#define XEL_TSR_PROG_MAC_ADDR   (XEL_TSR_XMIT_BUSY_MASK | XEL_TSR_PROGRAM_MASK)

/* Define for programming the MAC address into the EmacLite */
#define XEL_TSR_PROG_MAC_ADDR   (XEL_TSR_XMIT_BUSY_MASK | XEL_TSR_PROGRAM_MASK)

/* Receive Status Register (RSR) */
#define XEL_RSR_RECV_DONE_MASK  0x00000001      /* Rx complete */
#define XEL_RSR_RECV_IE_MASK    0x00000008      /* Rx interrupt enable bit */

/* Transmit Packet Length Register (TPLR) */
#define XEL_TPLR_LENGTH_MASK    0x0000FFFF      /* Tx packet length */

/* Receive Packet Length Register (RPLR) */
#define XEL_RPLR_LENGTH_MASK    0x0000FFFF      /* Rx packet length */

#define XEL_HEADER_OFFSET       12              /* Offset to length field */
#define XEL_HEADER_SHIFT        16              /* Shift value for length */

/* General Ethernet Definitions */
#define XEL_ARP_PACKET_SIZE             28      /* Max ARP packet size */
#define XEL_HEADER_IP_LENGTH_OFFSET     16      /* IP Length Offset */

// ETH APIs
extern void eth_init();

#endif
