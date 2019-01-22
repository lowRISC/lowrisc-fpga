// See LICENSE.Cambridge for license details.
// A hello world program

#include <stdio.h>
#include <stdint.h>
#include "lowrisc_memory_map.h"
#include "hid.h"
#include "mini-printf.h"
#include "minion_lib.h"
#include "lowrisc_memory_map.h"
#include "eth.h"

static const uip_ipaddr_t uip_broadcast_addr =
  { { 0xff, 0xff, 0xff, 0xff } };

static const uip_ipaddr_t uip_all_zeroes_addr = { { 0x0, /* rest is 0 */ } };

static uip_lladdr_t uip_lladdr;

// #define SIMULATION
#define VERBOSE
#define PORT 8888   //The port on which to send data

static inline void eth_write(size_t addr, uint64_t data)
{
  eth_base[addr >> 3] = data;
}

static inline uint64_t eth_read(size_t addr)
{
  return eth_base[addr >> 3];
}

static inline void eth_write_verbose(size_t addr, uint64_t data)
{
#ifndef SIMULATION  
  printf("eth_write(%x,%x)\n", addr, data);
#endif  
  eth_write(addr, data);
}

static inline uint64_t eth_read_verbose(size_t addr)
{
  uint64_t retval = eth_read(addr);
#ifndef SIMULATION  
  printf("eth_read(%x) returned %x\n", addr, retval);
#endif  
  return retval;
}

void lite_queue(const void *buf, int length)
{
  int i, busy;
  int rnd = ((length-1|7)+1);
  const uint64_t *alloc = buf;
#ifdef VERBOSE
  printf("TX pending\n");
#endif
  do busy = eth_read(TPLR_OFFSET);
  while (TPLR_BUSY_MASK & busy);
  for (i = 0; i < rnd/8; i++)
    {
      eth_write(TXBUFF_OFFSET+(i<<3), alloc[i]);
    }
  eth_write(TPLR_OFFSET,length);
}

extern uip_eth_addr mac_addr;
extern uint16_t __bswap_16(uint16_t x);
extern uint32_t __bswap_32(uint32_t x);

int main() {
  int dhcp_off_cnt = 0;
  int dhcp_ack_cnt = 0;
  uint32_t macaddr_lo, macaddr_hi;
  volatile int n, t = 0;
#ifndef SIMULATION  
  printf("Setup MAC addr\n");
#endif  
  mac_addr.addr[0] = (uint8_t)0xEE;
  mac_addr.addr[1] = (uint8_t)0xE1;
  mac_addr.addr[2] = (uint8_t)0xE2;
  mac_addr.addr[3] = (uint8_t)0xE3;
  mac_addr.addr[4] = (uint8_t)0xE4;
  mac_addr.addr[5] = (uint8_t)0xE0;
  uip_setethaddr(mac_addr);

  memcpy (&macaddr_lo, mac_addr.addr+2, sizeof(uint32_t));
  memcpy (&macaddr_hi, mac_addr.addr+0, sizeof(uint16_t));
  eth_write(MACLO_OFFSET, __bswap_32(macaddr_lo));
  eth_write(MACHI_OFFSET, __bswap_16(macaddr_hi));

  macaddr_lo = eth_read(MACLO_OFFSET);
  macaddr_hi = eth_read(MACHI_OFFSET) & MACHI_MACADDR_MASK;
  eth_write(MACHI_OFFSET, /*MACHI_ALLPKTS_MASK|*/macaddr_hi);
  eth_write(RFCS_OFFSET, 8); /* use 8 buffers */
  
#ifndef SIMULATION  
  printf("%x %x\n", macaddr_hi, macaddr_lo);
#endif
  printf("waiting\n");
  for (n = 0; n < 1000000; n++)
    t = t + n;
  printf("calling DHCP\n");
  dhcp_main(mac_addr.addr);
  for (;;)
    {
      if (eth_read(RSR_OFFSET) & RSR_RECV_DONE_MASK)
        {
          int rsr = eth_read(RSR_OFFSET);
          int buf = rsr & RSR_RECV_FIRST_MASK;
          int errs = eth_read(RBAD_OFFSET);
          int xlength = eth_read(RPLR_OFFSET+((buf&7)<<3)) & RPLR_LENGTH_MASK;
          if ((xlength >= 14) && (xlength <= max_packet))
            {
              int i, start = (RXBUFF_OFFSET>>3) + ((buf&7)<<8);
              uint32_t *alloc32_io = (uint32_t *)(eth_base+start);
              uint32_t alloc32[ETH_FRAME_LEN/sizeof(uint32_t)+1];
              int rnd = ((xlength-1|3)+1); /* round to a multiple of 4 */
              // Do we need to read the packet at all ??
              uint16_t rxheader = alloc32_io[HEADER_OFFSET >> 2];
              int proto_type = ntohs(rxheader) & 0xFFFF;
              for (i = 0; i < rnd/4; i++) alloc32[i] = alloc32_io[i];
              printf("length = %d (buf = %x) proto=0x%X\n", xlength, buf, proto_type);
              switch (proto_type)
                {
                case ETH_P_IP:
                  {
                    uint32_t peer_ip;
                    static u_char peer_addr[6];
                    struct ethip_hdr {
                      struct uip_eth_hdr ethhdr;
                      /* IP header. */
                      uint8_t vhl,
                        tos,
                        len[2],
                        ipid[2],
                        ipoffset[2],
                        ttl,
                        proto;
                      uint16_t ipchksum;
                      uip_ipaddr_t srcipaddr, destipaddr;
                      uint8_t body[];
                    } *BUF = ((struct ethip_hdr *)alloc32);
                    memcpy(&peer_ip, &(BUF->srcipaddr), sizeof(uip_ipaddr_t));
                    memcpy(peer_addr, BUF->ethhdr.src.addr, 6);
      #ifdef VERBOSE
                    printf("IP proto = %d\n", BUF->proto);
                    printf("Source IP Address:  %d.%d.%d.%d\n", uip_ipaddr_to_quad(&(BUF->srcipaddr)));
                    printf("Destination IP Address:  %d.%d.%d.%d\n", uip_ipaddr_to_quad(&(BUF->destipaddr)));
      #endif
                    switch (BUF->proto)
                      {
                      case IPPROTO_ICMP:
                        {
                          struct icmphdr
                          {
                            uint8_t type;		/* message type */
                            uint8_t code;		/* type sub-code */
                            uint16_t checksum;
                            uint16_t	id;
                            uint16_t	sequence;
                            uint64_t	timestamp;	/* gateway address */
                            uint8_t body[];
                          } *icmp_hdr = (struct icmphdr *)&(BUF->body);
      #ifdef VERBOSE
                        printf("IP proto = ICMP\n");
      #endif
                        write_led(BUF->proto);
                        if (uip_ipaddr_cmp(&BUF->destipaddr, &uip_hostaddr))
                          {
                            uint16_t chksum;
                            int hlen = sizeof(struct icmphdr);
                            int len = xlength - sizeof(struct ethip_hdr);
                            memcpy(BUF->ethhdr.dest.addr, BUF->ethhdr.src.addr, 6);
                            memcpy(BUF->ethhdr.src.addr, uip_lladdr.addr, 6);

                            uip_ipaddr_copy(&BUF->destipaddr, &BUF->srcipaddr);
                            uip_ipaddr_copy(&BUF->srcipaddr, &uip_hostaddr);

                            icmp_hdr->type = 0; /* reply */
                            icmp_hdr->checksum = 0;
                            chksum = csum((uint8_t *)icmp_hdr, len);
                            icmp_hdr->checksum = htons(chksum);
      #ifdef VERBOSE                      
                            printf("sending ICMP reply (header = %d, total = %d, checksum = %x)\n", hlen, len, chksum);
                            PrintData((u_char *)icmp_hdr, len);
      #endif                      
                            lite_queue(alloc32, xlength);
                          }
                        }
                        break;
                      case    IPPROTO_IGMP:
      #ifdef VERBOSE
                        printf("IP Proto = IGMP\n");
      #else
                        printf("G");
      #endif
                        break;
                      case    IPPROTO_IPIP: printf("IP Proto = IPIP\n"); break;
                      case    IPPROTO_TCP: printf("IP Proto = TCP\n"); break;
                      case    IPPROTO_EGP: printf("IP Proto = EGP\n"); break;
                      case    IPPROTO_PUP: printf("IP Proto = PUP\n"); break;
                      case    IPPROTO_UDP:
                        {
                          struct udphdr {
                            uint16_t	uh_sport;		/* source port */
                            uint16_t	uh_dport;		/* destination port */
                            uint16_t	uh_ulen;		/* udp length */
                            uint16_t	uh_sum;			/* udp checksum */
                            const u_char body[];              /* payload */
                          } *udp_hdr = (struct udphdr *)&(BUF->body);

                          int16_t dport = ntohs(udp_hdr->uh_dport);
                          int16_t ulen = ntohs(udp_hdr->uh_ulen);
                          uint16_t peer_port = ntohs(udp_hdr->uh_sport);
      #ifdef VERBOSE
                          printf("IP Proto = UDP, source port = %d, dest port = %d, length = %d\n",
                                 ntohs(udp_hdr->uh_sport),
                                 dport,
                                 ulen);
      #endif                        
                          if (dport == PORT)
                            {
                              process_udp_packet(udp_hdr->body, ulen-sizeof(struct udphdr), peer_port, peer_ip, peer_addr);
                            }
                          else if (peer_port == DHCP_SERVER_PORT)
                            {
                              if (!(dhcp_off_cnt && dhcp_ack_cnt))
                                dhcp_input((dhcp_t *)(udp_hdr->body), mac_addr.addr, &dhcp_off_cnt, &dhcp_ack_cnt);
                            }
                          else
                            {
      #ifdef VERBOSE
                              printf("IP Proto = UDP, source port = %d, dest port = %d, length = %d\n",
                                 ntohs(udp_hdr->uh_sport),
                                 dport,
                                 ulen);
      #endif                        
                            }
                        }
                        break;
                      case    IPPROTO_IDP: printf("IP Proto = IDP\n"); break;
                      case    IPPROTO_TP: printf("IP Proto = TP\n"); break;
                      case    IPPROTO_DCCP: printf("IP Proto = DCCP\n"); break;
                      case    IPPROTO_IPV6:
      #ifdef VERBOSE
                        printf("IP Proto = IPV6\n");
      #else
                        printf("6");
      #endif                      
                        break;
                      case    IPPROTO_RSVP: printf("IP Proto = RSVP\n"); break;
                      case    IPPROTO_GRE: printf("IP Proto = GRE\n"); break;
                      case    IPPROTO_ESP: printf("IP Proto = ESP\n"); break;
                      case    IPPROTO_AH: printf("IP Proto = AH\n"); break;
                      case    IPPROTO_MTP: printf("IP Proto = MTP\n"); break;
                      case    IPPROTO_BEETPH: printf("IP Proto = BEETPH\n"); break;
                      case    IPPROTO_ENCAP: printf("IP Proto = ENCAP\n"); break;
                      case    IPPROTO_PIM:
      #ifdef VERBOSE
                        printf("IP Proto = PIM\n");
      #else
                        printf("M");
      #endif
                        break;
                      case    IPPROTO_COMP: printf("IP Proto = COMP\n"); break;
                      case    IPPROTO_SCTP: printf("IP Proto = SCTP\n"); break;
                      case    IPPROTO_UDPLITE: printf("IP Proto = UDPLITE\n"); break;
                      case    IPPROTO_MPLS: printf("IP Proto = MPLS\n"); break;
                      case    IPPROTO_RAW: printf("IP Proto = RAW\n"); break;
                      default:
                        printf("IP proto = unsupported (%x)\n", BUF->proto);
                        break;
                      }
                  }
                  break;
                case ETH_P_ARP:
                  {
                    struct arp_hdr {
                      struct uip_eth_hdr ethhdr;
                      uint16_t hwtype;
                      uint16_t protocol;
                      uint8_t hwlen;
                      uint8_t protolen;
                      uint16_t opcode;
                      struct uip_eth_addr shwaddr;
                      uip_ipaddr_t sipaddr;
                      struct uip_eth_addr dhwaddr;
                      uip_ipaddr_t dipaddr;
                    } *BUF = ((struct arp_hdr *)alloc32);
      #ifdef VERBOSE
                   printf("proto = ARP\n");
      #endif
                   if(uip_ipaddr_cmp(&BUF->dipaddr, &uip_hostaddr))
                     {
                      int len = sizeof(struct arp_hdr);
                      BUF->opcode = __htons(2);

                      memcpy(BUF->dhwaddr.addr, BUF->shwaddr.addr, 6);
                      memcpy(BUF->shwaddr.addr, uip_lladdr.addr, 6);
                      memcpy(BUF->ethhdr.src.addr, uip_lladdr.addr, 6);
                      memcpy(BUF->ethhdr.dest.addr, BUF->dhwaddr.addr, 6);

                      uip_ipaddr_copy(&BUF->dipaddr, &BUF->sipaddr);
                      uip_ipaddr_copy(&BUF->sipaddr, &uip_hostaddr);

                      BUF->ethhdr.type = __htons(UIP_ETHTYPE_ARP);

      #ifdef VERBOSE
                      printf("sending ARP reply (length = %d)\n", len);
      #endif
                      lite_queue(alloc32, len);
                     }
                   else
                     {
      #ifdef VERBOSE
                       printf("Discarded ARP  %d.%d.%d.%d, my addr =  %d.%d.%d.%d\n", uip_ipaddr_to_quad(&BUF->dipaddr), uip_ipaddr_to_quad(&uip_hostaddr));
      #endif                 
                     }
                  }
                  break;
                case ETH_P_IPV6:
      #ifdef VERBOSE                      
                  printf("proto_type = IPV6\n");
      #else
                  printf("6");
      #endif                      
                  break;
                default:
                  printf("proto_type = 0x%x\n", proto_type);
                  break;
                }
            }
          eth_write(RSR_OFFSET, buf+1); /* acknowledge */
          //          printf("Alive %d\n", ++n);
        }
    }
}

void external_interrupt(void)
{
  int claim, handled = 0;
#ifdef VERBOSE
  printf("Hello external interrupt! "__TIMESTAMP__"\n");
#endif  
}
