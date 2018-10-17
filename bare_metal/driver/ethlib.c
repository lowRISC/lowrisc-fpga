/*
 * Copyright (c) 2001-2003, Adam Dunkels.
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is derived from the uIP TCP/IP stack.
 *
 *
 */

// An ethernet loader program
//#define VERBOSE
#include "encoding.h"
#include "bits.h"
#include "elfriscv.h"
#include "hid.h"
#include "lowrisc_memory_map.h"
#include "eth.h"
#include "mini-printf.h"
#include "minion_lib.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sdhci-minion-hash-md5.h"

const uip_ipaddr_t uip_broadcast_addr =
  { { 0xff, 0xff, 0xff, 0xff } };

const uip_ipaddr_t uip_all_zeroes_addr = { { 0x0, /* rest is 0 */ } };

uip_lladdr_t uip_lladdr;

//#define VERBOSE
//#define UDP_DEBUG

static inline void eth_write(size_t addr, uint64_t data)
{
#ifdef DEBUG
  if ((addr < 0x8000) && !(addr&7))
#endif    
    {
#ifdef VERBOSE
      printf("eth_write(%x,%x)\n", addr, data);
#endif      
      eth_base[addr >> 3] = data;
    }
#ifdef DEBUG
  else
    printf("eth_write(%x,%x) out of range\n", addr, data);
#endif  
}

static inline uint64_t eth_read(size_t addr)
{
  uint64_t retval = 0xDEADBEEF;
#ifdef DEBUG
  if ((addr < 0x8000) && !(addr&7))
#endif  
    {
      retval = eth_base[addr >> 3];
#ifdef VERBOSE
      printf("eth_read(%x) returned %x\n", addr, retval);
#endif      
    }
#ifdef DEBUG  
  else
    printf("eth_read(%x) out of range\n", addr);
#endif  
  return retval;
}

/* General Ethernet Definitions */
#define ARP_PACKET_SIZE		28	/* Max ARP packet size */
#define HEADER_IP_LENGTH_OFFSET	16	/* IP Length Offset */

#define ETH_DATA_LEN	1500		/* Max. octets in payload	 */
#define ETH_P_IP	0x0800		/* Internet Protocol packet	*/
#define ETH_HLEN	14		/* Total octets in header.	 */
#define ETH_FCS_LEN	4		/* Octets in the FCS		 */
#define ETH_P_ARP	0x0806		/* Address Resolution packet	*/
#define ETH_P_IPV6      0x86DD          /* IPv6 */
#define ETH_FRAME_LEN	1514		/* Max. octets in frame sans FCS */

extern uip_eth_addr mac_addr;

void *mysbrk(size_t len)
{
  static unsigned long rused = 0;
  char *rd = rused + (char *)get_ddr_base() +  ((uint64_t)get_ddr_size()) / 2;
  rused += ((len-1)|7)+1;
  return rd;
}


#define PORT 8888   //The port on which to send data
#define CHUNK_SIZE 1464

void process_ip_packet(const u_char *, int);
void print_ip_packet(const u_char * , int);
void print_tcp_packet(const u_char * , int);
void process_udp_packet(const u_char *, int, uint16_t, uint32_t, const u_char *);
void PrintData (const u_char * , int);

#define min(x,y) (x) < (y) ? (x) : (y)

int eth_discard = 0;
void process_my_packet(int size, const u_char *buffer);

static int copyin_pkt(void)
{
  int i, last;
  int rsr = eth_read(RSR_OFFSET);
  int buf = rsr & RSR_RECV_FIRST_MASK;
  int errs = eth_read(RBAD_OFFSET);
  int len = eth_read(RPLR_OFFSET+((buf&7)<<3)) & RPLR_LENGTH_MASK;
#ifdef VERBOSE
      printf("length = %d (buf = %x)\n", len, buf);
#endif      
      if ((len >= 14) && (len <= max_packet) && ((0x101<<(buf&7)) & ~errs) && !eth_discard)
    {
      int rnd, start = (RXBUFF_OFFSET>>3) + ((buf&7)<<8);
      uint64_t *alloc = rxbuf[rxhead].alloc;
      uint32_t *alloc32 = (uint32_t *)(eth_base+start);
      // Do we need to read the packet at all ??
      uint16_t rxheader = alloc32[HEADER_OFFSET >> 2];
      int proto_type = ntohs(rxheader) & 0xFFFF;
      switch (proto_type)
          {
          case ETH_P_IP:
          case ETH_P_ARP:
            rnd = ((len-1|7)+1); /* round to a multiple of 8 */
            for (i = 0; i < rnd/8; i++)
              {
                alloc[i] = eth_base[start+i];
              }
            rxbuf[rxhead].len = len;
            rxhead = (rxhead + 1) % queuelen;
            break;            
          case ETH_P_IPV6:
            break;
          }
    }
  eth_write(RSR_OFFSET, buf+1); /* acknowledge */
  return len;
}

// max size of file image is increased to 17M to support FreeBSD downloading
#define MAX_FILE_SIZE (sizeof_maskarray*8*CHUNK_SIZE)

// size of DDR RAM (128M for NEXYS4-DDR) 
#define DDR_SIZE 0x8000000

enum {sizeof_maskarray=CHUNK_SIZE};

static int oldidx;
static uint64_t maskarray[sizeof_maskarray/sizeof(uint64_t)];
static const char *const regnam(int ix)
{
 switch (ix)
   {
   case MACLO_OFFSET: return "MACLO_OFFSET";
   case MACHI_OFFSET: return "MACHI_OFFSET";
   case TPLR_OFFSET: return "TPLR_OFFSET";
   case TFCS_OFFSET: return "TFCS_OFFSET";
   case MDIOCTRL_OFFSET: return "MDIOCTRL_OFFSET";
   case RFCS_OFFSET: return "RFCS_OFFSET";
   case RSR_OFFSET: return "RSR_OFFSET";
   case RBAD_OFFSET: return "RBAD_OFFSET";
   case RPLR_OFFSET: return "RPLR_OFFSET";
   default: if (ix < RPLR_OFFSET+64)
       {
         static char nam[20];
         sprintf(nam, "RPLR_OFFSET+%d", ix-RPLR_OFFSET);
         return nam;
       }
     else return "????";
   }
  
};

static void dumpregs(const char *msg)
{
  int i;
#ifdef VERBOSE  
  printf("== %s ==\n", msg);
  for (i = MACLO_OFFSET; i < RPLR_OFFSET+64; i+=8)
    {
      printf("eth_read(%s) = %x\n", regnam(i), eth_read(i));
    }
#endif  
}

void external_interrupt(void)
{
  int claim, handled = 0;
#ifdef VERBOSE
  printf("Hello external interrupt! "__TIMESTAMP__"\n");
#endif  
  claim = plic[0x80001];
  //  eth_write(MACHI_OFFSET, eth_read(MACHI_OFFSET)&~MACHI_IRQ_EN);
  dumpregs("before");
  /* Check if there is Rx Data available */
  while (eth_read(RSR_OFFSET) & RSR_RECV_DONE_MASK)
    {
#ifdef VERBOSE
      printf("Ethernet interrupt\n");
#endif  
      int length = copyin_pkt();
      handled = 1;
    }
  if (hid_check_read_irq())
    {
      int rslt = hid_read_irq();
      printf("uart interrupt read %x (%c)\n", rslt, rslt);
      handled = 1;
    }
  if (!handled)
    {
      printf("unhandled interrupt!\n");
    }
  dumpregs("after");
  plic[0x80001] = claim;
  //  eth_write(MACHI_OFFSET, eth_read(MACHI_OFFSET)|MACHI_IRQ_EN);
}
    // Function for checksum calculation. From the RFC,
    // the checksum algorithm is:
    //  "The checksum field is the 16 bit one's complement of the one's
    //  complement sum of all 16 bit words in the header.  For purposes of
    //  computing the checksum, the value of the checksum field is zero."

static unsigned short csum(uint8_t *buf, int nbytes)
    {       //
            unsigned long sum;
            for(sum=0; nbytes>0; nbytes-=2)
              {
                unsigned short src;
                memcpy(&src, buf, 2);
                buf+=2;
                sum += ntohs(src);
              }
            sum = (sum >> 16) + (sum & 0xffff);
            sum += (sum >> 16);
            return (unsigned short)(~sum);
    }

static uintptr_t old_mstatus, old_mie;

static uint64_t random_pkt[max_packet/sizeof(uint64_t)];

void loopback_test(int loops, int sim)
  {
    int j;
    uint64_t hi = eth_read(MACHI_OFFSET) & MACHI_MACADDR_MASK;
    eth_write(MACHI_OFFSET, MACHI_LOOPBACK_MASK|hi);
    for (j = 1; j <= loops; j++)
      {
	enum {maxcnt=187};
	int i, waiting, actualwait, match = 0, waitcnt = 0, errcnt = 0;
	int tstcnt = 1 << j;
        int buf = (eth_read(RSR_OFFSET) & RSR_RECV_NEXT_MASK) >> 4;
        int start = RXBUFF_OFFSET + ((buf&7)<<11);
	if (tstcnt > maxcnt) tstcnt = maxcnt; /* max length packet */
	if (!sim) printf("Selftest iteration %d, next buffer = %d, rx_start = %x\n", j, buf, start);
      /* bit-level digital loopback */
        eth_write(RFCS_OFFSET, 8); /* use 8 buffers */
        eth_write(RSR_OFFSET, buf); /* clear pending receive packet, if any */
      /* random inits */
        if (!sim)
          {
            random_pkt[sizeof(random_pkt)/sizeof(uint64_t)-1] = rand64();
            for (i = 0; i < tstcnt*8; i += 8) random_pkt[i >> 3] = rand64();
          }
        random_pkt[0] = 0xFFFFFFFFFFFFFFFF;
        random_pkt[1] = 0x55555555DEADBEEF;
        for (i = 0; i < tstcnt*8; i += 8)  
          {
            eth_write(TXBUFF_OFFSET+i, random_pkt[i >> 3]);
            eth_write(start+i, i);
          }
        for (i = 0; i < tstcnt*8; i += 8) if (errcnt < 10)
          {
            uint64_t chk1 = eth_read(TXBUFF_OFFSET+i);
            uint64_t chk2 = eth_read(start+i);
            if (chk1 != random_pkt[i >> 3])
              {
                ++errcnt;
                printf("TX Buffer offset %d: write=%x, read=%x\n", i, random_pkt[i >> 3], chk1);
              }
            if (chk2 != i)
              {
                ++errcnt;
                printf("RX Buffer offset %d: write=%x, read=%x\n", i, i, chk2);
              }
          }
      /* launch the packet */
      eth_write(TPLR_OFFSET, tstcnt*8);
      /* wait for loopback to do its work */
      do 
	{
	  waiting = ((eth_read(RSR_OFFSET) & RSR_RECV_DONE_MASK) == 0);
	  if (waiting) actualwait = waitcnt;
	}
      while ((waitcnt++ < tstcnt) || waiting);
      for (i = 0; i < tstcnt; i++)
	{
          uint64_t xmit_rslt = random_pkt[i];
          uint64_t recv_rslt = eth_read(start+(i<<3));
	  if (xmit_rslt != recv_rslt)
	    {
              if ((i < 10) && !sim)
                {
                  int fcs = eth_read(RFCS_OFFSET);
                  printf("Buffer offset %d: xmit=%x, recv=%x, fcs=%x\n", i, xmit_rslt, recv_rslt, fcs);
                }
	    }
	  else
	    ++match;
	}
      if (!sim)
      printf("Selftest matches=%d/%d, delay = %d\n", match, tstcnt, actualwait);
      }
  }

int eth_main(void) {
  int dhcp_off_cnt = 0;
  int dhcp_ack_cnt = 0;
  uip_ipaddr_t addr;
  uint64_t lo = eth_read(MACLO_OFFSET);
  uint64_t hi = eth_read(MACHI_OFFSET) & MACHI_MACADDR_MASK;
  eth_write(MACHI_OFFSET, MACHI_IRQ_EN|hi);
  rxbuf = (inqueue_t *)mysbrk(sizeof(inqueue_t)*queuelen);
  txbuf = (outqueue_t *)mysbrk(sizeof(outqueue_t)*queuelen);
  printf("Max file size is %d bytes\n", MAX_FILE_SIZE);
  //  maskarray = (uint64_t *)mysbrk(sizeof_maskarray);
  memset(maskarray, 0, sizeof_maskarray);
  
#ifndef VERBOSE  
  printf("MAC = %x:%x\n", hi&MACHI_MACADDR_MASK, lo);
  
  printf("MAC address = %02x:%02x:%02x:%02x:%02x:%02x.\n",
         mac_addr.addr[0],
         mac_addr.addr[1],
         mac_addr.addr[2],
         mac_addr.addr[3],
         mac_addr.addr[4],
         mac_addr.addr[5]
         );
#endif
  uip_setethaddr(mac_addr);
#ifdef INTERRUPT_MODE
  printf("Enabling interrupts\n");
  old_mstatus = read_csr(mstatus);
  old_mie = read_csr(mie);
  set_csr(mstatus, MSTATUS_MIE|MSTATUS_HIE);
  set_csr(mie, ~(1 << IRQ_M_TIMER));

  printf("Enabling UART interrupt\n");
  hid_enable_read_irq();
#endif
  write_led(-1);
  dhcp_main(mac_addr.addr);
  do {
#ifndef INTERRUPT_MODE
    while (eth_read(RSR_OFFSET) & RSR_RECV_DONE_MASK) copyin_pkt();
#endif
    if ((txhead != txtail) && (TPLR_BUSY_MASK & ~eth_read(TPLR_OFFSET)))
      {
        uint64_t *alloc = txbuf[txtail].alloc;
        int length = txbuf[txtail].len;
        int i, rslt;
#ifdef VERBOSE
        printf("TX pending\n");
#endif
        for (i = 0; i < ((length-1|7)+1)/8; i++)
          {
            eth_write(TXBUFF_OFFSET+(i<<3), alloc[i]);
          }
        eth_write(TPLR_OFFSET,length);
        txtail = (txtail + 1) % queuelen;
      }
#ifndef INTERRUPT_MODE
    while (eth_read(RSR_OFFSET) & RSR_RECV_DONE_MASK) copyin_pkt();
#endif
    if (rxhead != rxtail)
      {
	int i, bad = 0;
        uint32_t *alloc32 = (uint32_t *)(rxbuf[rxtail].alloc); // legacy size to avoid tweaking header offset
        int length, xlength = rxbuf[rxtail].len;
        uint16_t rxheader = alloc32[HEADER_OFFSET >> 2];
        int proto_type = ntohs(rxheader) & 0xFFFF;
#ifdef VERBOSE
        printf("alloc32 = %x\n", alloc32);
        printf("rxhead = %d, rxtail = %d\n", rxhead, rxtail);
#endif
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
                      lite_queue(alloc32, xlength+4);
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
        rxtail = (rxtail + 1) % queuelen;
      }
  } while (1);
}

static void ethboot(void)
{
  uint32_t br;
  uint8_t *memory_base = (uint8_t *)(get_ddr_base());
  printf("Disabling interrupts\n");
  write_csr(mie, old_mie);
  write_csr(mstatus, old_mstatus);
  hid_disable_read_irq();
  eth_write(MACHI_OFFSET, eth_read(MACHI_OFFSET)&~MACHI_IRQ_EN);
  eth_write(RSR_OFFSET, RSR_RECV_LAST_MASK);
  printf("Ethernet interrupt status = %d\n", eth_read(RSR_OFFSET));

  printf("Boot the loaded program...\n");

  uintptr_t mstatus = read_csr(mstatus);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPP, PRV_M);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPIE, 1);
  write_csr(mstatus, mstatus);
  write_csr(mepc, memory_base);

  printf("Goodbye, booter ...\n");
  asm volatile ("fence.i");
  asm volatile ("mret");
}

void process_udp_packet(const u_char *data, int ulen, uint16_t peer_port, uint32_t peer_ip, const u_char *peer_addr)
{
  uint16_t idx;	
  static uint16_t maxidx;
  uint64_t siz = ((uint64_t)get_ddr_size());
  uint8_t *boot_file_buf = (uint8_t *)(get_ddr_base()) + siz - MAX_FILE_SIZE;
  uint8_t *boot_file_buf_end = (uint8_t *)(get_ddr_base()) + siz;
  uint32_t srcaddr;
  memcpy(&srcaddr, &uip_hostaddr, 4);
  if (ulen == CHUNK_SIZE+sizeof(uint16_t))
    {
      memcpy(&idx, data+CHUNK_SIZE, sizeof(uint16_t));
#ifdef VERBOSE
      printf("idx = %x\n", idx);
#endif
      switch (idx)
        {
        case 0xFFFF:
          {
            printf("Boot requested\n");
            ethboot();
            break;
          }
        case 0xFFFE:
          {
            oldidx = 0;
            maxidx = 0;
            printf("Clear blocks requested\n");
            memset(maskarray, 0, sizeof_maskarray);
            udp_send(mac_addr.addr, maskarray, sizeof_maskarray, PORT, peer_port, srcaddr, peer_ip, peer_addr);
            break;
          }
        case 0xFFFD:
          {
            printf("Report blocks requested\n");
            udp_send(mac_addr.addr, maskarray, sizeof_maskarray, PORT, peer_port, srcaddr, peer_ip, peer_addr);
            break;
          }
        case 0xFFFC:
          {
            uint8_t *digest;
            uint32_t *target;
            size_t siz;
            memcpy(&target, data, sizeof(uint32_t *));
            memcpy(&siz, data+sizeof(uint32_t *), sizeof(siz));
            printf("Copy and report MD5 of size %d of target memory %p\n", siz, target);
            memcpy(target, boot_file_buf, siz);
            digest = hash_buf(target, siz);
            udp_send(mac_addr.addr, digest, hash_length * 2 + 1, PORT, peer_port, srcaddr, peer_ip, peer_addr);
            break;
          }
        case 0xFFFB:
          {
            int j;
            uint8_t *fakedigest;
            uint32_t *target;
            size_t siz;
            memcpy(&target, data, sizeof(uint32_t *));
            memcpy(&siz, data+sizeof(uint32_t *), sizeof(siz));
            printf("Copying chunk of size %d to target memory %p\n", siz, target);
            memcpy(target, boot_file_buf, siz);
	    fakedigest = data+sizeof(uint32_t *)+sizeof(size_t);
            udp_send(mac_addr.addr, fakedigest, hash_length * 2 + 1, PORT, peer_port, srcaddr, peer_ip, peer_addr);
            break;
          }
        case 0xFFFA:
          {
            void *target;
            size_t siz;
            memcpy(&target, data, sizeof(target));
            memcpy(&siz, data+sizeof(void *), sizeof(siz));
            printf("Clearing chunk of size %d to target memory %p\n", siz, target);
            memset(target, 0, siz);
            break;
          }
        case 0xFFF9:
          {
            void *target;
            size_t siz;
            memcpy(&target, data, sizeof(target));
            memcpy(&siz, data+sizeof(void *), sizeof(siz));
            printf("Downloading chunk of size %d from target memory %p\n", siz, target);
            udp_send(mac_addr.addr, target, siz, PORT, peer_port, srcaddr, peer_ip, peer_addr);
            break;
          }
        default:
          {
            uint8_t *boot_file_ptr = boot_file_buf+idx*CHUNK_SIZE;
            if (boot_file_ptr+CHUNK_SIZE < boot_file_buf_end)
              {
                memcpy(boot_file_ptr, data, CHUNK_SIZE);
                maskarray[idx/64] |= 1ULL << (idx&63);
                if (maxidx <= idx)
                  maxidx = idx+1;
              }
            else
              printf("Data Payload index %d out of range\n", idx);
#ifdef VERBOSE
            printf("Data Payload index %d\n", idx);
#else
            if (idx % 100 == 0) printf(".");
#endif
            oldidx = idx;
          }
        }
    }
  else
    {
      printf("UDP packet length %d sent to port %d\n", ulen, peer_port);
#ifdef UDP_DEBUG
      PrintData(data, ulen);
#endif      
      udp_send(mac_addr.addr, (void *)data, ulen, PORT, peer_port, srcaddr, peer_ip, peer_addr);
    }
}

#if defined(VERBOSE) || defined(UDP_DEBUG)
void PrintData (const u_char * data , int Size)
{
    int i , j;
    for(i=0 ; i < Size ; i++)
    {
        if( i!=0 && i%16==0)
        {
            printf("         ");
            for(j=i-16 ; j<i ; j++)
            {
                if(data[j]>=32 && data[j]<=128)
                    printf("%c",(unsigned char)data[j]);
                else printf(".");
            }
            printf("\n");
        }
        if(i%16==0) printf("   ");
            printf(" %02X",(unsigned int)data[i]);
        if( i==Size-1)
        {
            for(j=0;j<15-i%16;j++)
            {
              printf("   ");
            }
            printf("         ");
            for(j=i-i%16 ; j<=i ; j++)
            {
                if(data[j]>=32 && data[j]<=128)
                {
                  printf("%c",(unsigned char)data[j]);
                }
                else
                {
                  printf(".");
                }
            }
            printf("\n" );
        }
    }
}
#endif
