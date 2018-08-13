// See LICENSE for license details.

#ifndef ETH_HEADER_H
#define ETH_HEADER_H

#include <stdint.h>

/* Register offsets (in bytes) for the LowRISC Core */
#define TXBUFF_OFFSET       0x1000          /* Transmit Buffer */

#define MACLO_OFFSET        0x0800          /* MAC address low 32-bits */
#define MACHI_OFFSET        0x0808          /* MAC address high 16-bits and MAC ctrl */
#define TPLR_OFFSET         0x0810          /* Tx packet length */
#define TFCS_OFFSET         0x0818          /* Tx frame check sequence register */
#define MDIOCTRL_OFFSET     0x0820          /* MDIO Control Register */
#define RFCS_OFFSET         0x0828          /* Rx frame check sequence register(read) and last register(write) */
#define RSR_OFFSET          0x0830          /* Rx status and reset register */
#define RBAD_OFFSET         0x0838          /* Rx bad frame and bad fcs register arrays */
#define RPLR_OFFSET         0x0840          /* Rx packet length register array */

#define RXBUFF_OFFSET       0x4000          /* Receive Buffer */
#define MDIORD_RDDATA_MASK    0x0000FFFF    /* Data to be Read */

/* MAC Ctrl Register (MACHI) Bit Masks */
#define MACHI_MACADDR_MASK    0x0000FFFF     /* MAC high 16-bits mask */
#define MACHI_COOKED_MASK     0x00010000     /* obsolete flag */
#define MACHI_LOOPBACK_MASK   0x00020000     /* Rx loopback packets */
#define MACHI_ALLPKTS_MASK    0x00400000     /* Rx all packets (promiscuous mode) */
#define MACHI_IRQ_EN          0x00800000     /* Rx packet interrupt enable */

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
#define RSR_RECV_FIRST_MASK   0x0000000F      /* first available buffer (static) */
#define RSR_RECV_NEXT_MASK    0x000000F0      /* current rx buffer (volatile) */
#define RSR_RECV_LAST_MASK    0x00000F00      /* last available rx buffer (static) */
#define RSR_RECV_DONE_MASK    0x00001000      /* Rx complete */
#define RSR_RECV_IRQ_MASK     0x00002000      /* Rx irq bit */

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

enum {queuelen = 1024, max_packet = 1536};

enum
  {
    IPPROTO_IP = 0,
    IPPROTO_ICMP = 1,
    IPPROTO_IGMP = 2,
    IPPROTO_IPIP = 4,
    IPPROTO_TCP = 6,
    IPPROTO_EGP = 8,
    IPPROTO_PUP = 12,
    IPPROTO_UDP = 17,
    IPPROTO_IDP = 22,
    IPPROTO_TP = 29,
    IPPROTO_DCCP = 33,
    IPPROTO_IPV6 = 41,
    IPPROTO_RSVP = 46,
    IPPROTO_GRE = 47,
    IPPROTO_ESP = 50,
    IPPROTO_AH = 51,
    IPPROTO_MTP = 92,
    IPPROTO_BEETPH = 94,
    IPPROTO_ENCAP = 98,
    IPPROTO_PIM = 103,
    IPPROTO_COMP = 108,
    IPPROTO_SCTP = 132,
    IPPROTO_UDPLITE = 136,
    IPPROTO_MPLS = 137,
    IPPROTO_RAW = 255,
    IPPROTO_MAX
  };

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

typedef unsigned short __be16;

struct ethhdr {
 unsigned char h_dest[6];
 unsigned char h_source[6];
 __be16 h_proto;
} __attribute__((packed));

struct iphdr
  {
    unsigned int ihl:4;
    unsigned int version:4;
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
  };

struct udphdr
{
  __extension__ union
  {
    struct
    {
      uint16_t uh_sport;
      uint16_t uh_dport;
      uint16_t uh_ulen;
      uint16_t uh_sum;
    };
    struct
    {
      uint16_t source;
      uint16_t dest;
      uint16_t len;
      uint16_t check;
    };
  };
};

typedef unsigned int u_int8_t __attribute__ ((__mode__ (__QI__)));
typedef unsigned int u_int16_t __attribute__ ((__mode__ (__HI__)));
typedef unsigned int u_int32_t __attribute__ ((__mode__ (__SI__)));
typedef unsigned int u_int64_t __attribute__ ((__mode__ (__DI__)));
typedef unsigned short int u_short;

typedef uint32_t in_addr_t;
struct in_addr
  {
    in_addr_t s_addr;
  };

struct ip
  {
    unsigned int ip_hl:4;
    unsigned int ip_v:4;
    u_int8_t ip_tos;
    u_short ip_len;
    u_short ip_id;
    u_short ip_off;
    u_int8_t ip_ttl;
    u_int8_t ip_p;
    u_short ip_sum;
    struct in_addr ip_src, ip_dst;
  };

#define UIP_ETHTYPE_ARP  0x0806
#define UIP_ETHTYPE_IP   0x0800
#define UIP_ETHTYPE_IPV6 0x86dd
#define ETHERTYPE_IP UIP_ETHTYPE_IP
#define ETHER_ADDR_LEN   6
#define IPVERSION        4
#define PCAP_ERRBUF_SIZE 256
#define DHCP_CHADDR_LEN 16
#define DHCP_SNAME_LEN  64
#define DHCP_FILE_LEN   128
#define DHCP_SERVER_PORT    67
#define DHCP_CLIENT_PORT    68

typedef uip_eth_addr uip_lladdr_t;
typedef uint8_t u_char;

#if 0
uint16_t __bswap_16(uint16_t x);
uint32_t __bswap_32(uint32_t x);
#endif

#define ntohl(x) ({ uint32_t __tmp; \
      uint8_t *optr = (uint8_t *)&__tmp; \
      uint8_t *iptr = (uint8_t *)&(x); \
      int i; \
      for (i = 0; i < sizeof(uint32_t); i++) optr[sizeof(uint32_t)-i-1] = iptr[i]; \
      __tmp; })

#define ntohs(x) ({ uint16_t __tmp; \
      uint8_t *optr = (uint8_t *)&__tmp; \
      uint8_t *iptr = (uint8_t *)&(x); \
      int i; \
      for (i = 0; i < sizeof(uint16_t); i++) optr[sizeof(uint16_t)-i-1] = iptr[i]; \
      __tmp; })

#define htonl(x) ntohl(x)
#define htons(x) ntohs(x)

typedef unsigned int __u_int;
typedef __u_int bpf_u_int32;
typedef long int __time_t;
typedef long int __suseconds_t;
typedef unsigned int u_int16_t __attribute__ ((__mode__ (__HI__)));

struct ether_header
{
  u_int8_t ether_dhost[6];
  u_int8_t ether_shost[6];
  u_int16_t ether_type;
} __attribute__ ((__packed__));

struct __timeval
  {
    __time_t tv_sec;
    __suseconds_t tv_usec;
  };

struct pcap_pkthdr {
 struct __timeval ts;
 bpf_u_int32 caplen;
 bpf_u_int32 len;
};

typedef u_int32_t ip4_t;

/*
 * http://www.tcpipguide.com/free/t_DHCPMessageFormat.htm
 */
typedef struct dhcp
{
    u_int8_t    opcode;
    u_int8_t    htype;
    u_int8_t    hlen;
    u_int8_t    hops;
    u_int32_t   xid;
    u_int16_t   secs;
    u_int16_t   flags;
    ip4_t       ciaddr;
    ip4_t       yiaddr;
    ip4_t       siaddr;
    ip4_t       giaddr;
    u_int8_t    chaddr[DHCP_CHADDR_LEN];
    char        bp_sname[DHCP_SNAME_LEN];
    char        bp_file[DHCP_FILE_LEN];
    uint32_t    magic_cookie;
    u_int8_t    bp_options[0];
} dhcp_t;

typedef struct inqueue_t {
  uint64_t alloc[max_packet];
  uint64_t len;
} inqueue_t;

typedef struct outqueue_t {
  uint64_t alloc[max_packet];
  uint64_t len;
} outqueue_t;

extern uip_ipaddr_t uip_hostaddr, uip_draddr, uip_netmask;
extern volatile int rxhead, rxtail, txhead, txtail;
extern inqueue_t *rxbuf;
extern outqueue_t *txbuf;

int dhcp_main(u_int8_t mac[6]);
void lite_queue(const void *buf, int length);
void dhcp_input(dhcp_t *dhcp, u_int8_t mac[6], int *offcount, int *ackcount);
int udp_send(const u_int8_t *mac, void *msg, int payload_size, uint16_t client, uint16_t server, uint32_t srcaddr, uint32_t dstaddr, const u_int8_t *destmac);
void loopback_test(int loops, int sim);
int eth_main(void);

#endif
