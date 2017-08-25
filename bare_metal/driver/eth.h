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
#define TXBUFF_OFFSET       0x1000          /* Transmit Buffer */

#define MACLO_OFFSET        0x0800          /* MAC address low 32-bits */
#define MACHI_OFFSET        0x0804          /* MAC address high 16-bits and MAC ctrl */
#define TPLR_OFFSET         0x0808          /* Tx packet length */
#define TFCS_OFFSET         0x080C          /* Tx frame check sequence register */
#define MDIOCTRL_OFFSET     0x0810          /* MDIO Control Register */
#define RFCS_OFFSET         0x0814          /* Rx frame check sequence register */
#define RSR_OFFSET          0x0818          /* Rx status and reset register */
#define RPLR_OFFSET         0x081C          /* Rx packet length register */

#define RXBUFF_OFFSET       0x0004          /* Receive Buffer */
#define MDIORD_RDDATA_MASK    0x0000FFFF    /* Data to be Read */

/* MAC Ctrl Register (MACHI) Bit Masks */
#define MACHI_MACADDR_MASK    0x0000FFFF     /* MAC high 16-bits mask */
#define MACHI_COOKED_MASK     0x00010000     /* Rx cooked packets */
#define MACHI_LOOPBACK_MASK   0x00020000     /* Rx loopback packets */
#define MACHI_LOOPBACK2_MASK  0x00040000     /* Rx byte loopback packets */
#define MACHI_DATA_DLY_MASK   0x00180000     /* Rx packet data buffer alignment delay */

/* MDIO Control Register Bit Masks */
#define MDIOCTRL_MDIOCLK_MASK 0x00000001    /* MDIO Clock Mask */
#define MDIOCTRL_MDIOOUT_MASK 0x00000002    /* MDIO Output Mask */
#define MDIOCTRL_MDIOOEN_MASK 0x00000004    /* MDIO Output Enable Mask */
#define MDIOCTRL_MDIOIN_MASK  0x00000008    /* MDIO Input Mask */

/* Transmit Status Register (TPLR) Bit Masks */
#define TPLR_FRAME_ADDR_MASK  0xFFFF0000     /* Tx complete */
#define TPLR_PACKET_LEN_MASK  0x0000FFFF     /* Tx packet length */

/* Receive Status Register (RSR) */
#define RSR_RECV_DONE_MASK    0x00000001      /* Rx complete */
#define RSR_RECV_ERR_MASK     0x00000002      /* Rx fcs_err bit */

/* Transmit Packet Length Register (TPLR) */
#define TPLR_LENGTH_MASK    0x0000FFFF      /* Tx packet length */

/* Receive Packet Length Register (RPLR) */
#define RPLR_LENGTH_MASK    0x0000FFFF      /* Rx packet length */

/* General Ethernet Definitions */
#define HEADER_OFFSET       12              /* Offset to length field */
#define HEADER_SHIFT        16              /* Shift value for length */
#define ARP_PACKET_SIZE             28      /* Max ARP packet size */
#define HEADER_IP_LENGTH_OFFSET     16      /* IP Length Offset */

// ETH APIs
extern void eth_init();

#endif
