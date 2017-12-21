// See LICENSE for license details.

#ifndef ETH_HEADER_H
#define ETH_HEADER_H

#include <stdint.h>
#include "dev_map.h"

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

#define RXBUFF_OFFSET       0x0000          /* Receive Buffer */
#define MDIORD_RDDATA_MASK    0x0000FFFF    /* Data to be Read */

/* MAC Ctrl Register (MACHI) Bit Masks */
#define MACHI_MACADDR_MASK    0x0000FFFF     /* MAC high 16-bits mask */
#define MACHI_COOKED_MASK     0x00010000     /* obsolete flag */
#define MACHI_LOOPBACK_MASK   0x00020000     /* Rx loopback packets */
#define MACHI_IRQ_EN          0x00400000     /* Rx packet interrupt enable */

/* MDIO Control Register Bit Masks */
#define MDIOCTRL_MDIOCLK_MASK 0x00000001    /* MDIO Clock Mask */
#define MDIOCTRL_MDIOOUT_MASK 0x00000002    /* MDIO Output Mask */
#define MDIOCTRL_MDIOOEN_MASK 0x00000004    /* MDIO Output Enable Mask */
#define MDIOCTRL_MDIORST_MASK 0x00000008    /* MDIO Input Mask */
#define MDIOCTRL_MDIOIN_MASK  0x00000008    /* MDIO Input Mask */

/* Transmit Status Register (TPLR) Bit Masks */
#define TPLR_FRAME_ADDR_MASK  0x0FFF0000     /* Tx frame address */
#define TPLR_PACKET_LEN_MASK  0x00000FFF     /* Tx packet length */
#define TPLR_BUSY_MASK        0x80000000     /* Tx busy mask */

/* Receive Status Register (RSR) */
#define RSR_RECV_DONE_MASK    0x00000001      /* Rx complete */
#define RSR_RECV_IRQ_MASK     0x00000002      /* Rx irq bit */

/* Receive Packet Length Register (RPLR) */
#define RPLR_LENGTH_MASK      0x00000FFF      /* Rx packet length */
#define RPLR_ERROR_MASK       0x40000000      /* Rx error mask */
#define RPLR_FCS_ERROR_MASK   0x80000000      /* Rx FCS error mask */

/* General Ethernet Definitions */
#define HEADER_OFFSET               12      /* Offset to length field */
#define HEADER_SHIFT                16      /* Shift value for length */
#define ARP_PACKET_SIZE             28      /* Max ARP packet size */
#define HEADER_IP_LENGTH_OFFSET     16      /* IP Length Offset */

// ETH APIs

#define uip_sethostaddr(addr) uip_ipaddr_copy(&uip_hostaddr, (addr))
#define uip_setnetmask(addr) uip_ipaddr_copy(&uip_netmask, (addr))
#define uip_ipaddr_to_quad(a) (a)->u8[0],(a)->u8[1],(a)->u8[2],(a)->u8[3]
#define uip_ipaddr_copy(dest, src) (*(dest) = *(src))
#define uip_ipaddr_cmp(addr1, addr2) ((addr1)->u16[0] == (addr2)->u16[0] && \
                                       (addr1)->u16[1] == (addr2)->u16[1])

#define uip_ipaddr(addr, addr0,addr1,addr2,addr3) do {  \
    (addr)->u8[0] = addr0;                              \
    (addr)->u8[1] = addr1;                              \
    (addr)->u8[2] = addr2;                              \
    (addr)->u8[3] = addr3;                              \
  } while(0)

#define uip_setethaddr(eaddr) do {uip_lladdr.addr[0] = eaddr.addr[0]; \
                              uip_lladdr.addr[1] = eaddr.addr[1];\
                              uip_lladdr.addr[2] = eaddr.addr[2];\
                              uip_lladdr.addr[3] = eaddr.addr[3];\
                              uip_lladdr.addr[4] = eaddr.addr[4];\
                              uip_lladdr.addr[5] = eaddr.addr[5];} while(0)

extern void eth_init();

typedef union uip_ip4addr_t {
  uint8_t  u8[4];                       /* Initializer, must come first. */
  uint16_t u16[2];
} uip_ip4addr_t;

typedef uip_ip4addr_t uip_ipaddr_t;

typedef struct uip_eth_addr {
  uint8_t addr[6];
} uip_eth_addr;

struct uip_eth_hdr {
  struct uip_eth_addr dest;
  struct uip_eth_addr src;
  uint16_t type;
};

#define UIP_ETHTYPE_ARP  0x0806
#define UIP_ETHTYPE_IP   0x0800
#define UIP_ETHTYPE_IPV6 0x86dd

typedef uip_eth_addr uip_lladdr_t;
typedef uint8_t u_char;

#endif
