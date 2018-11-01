// An ethernet loader program
// #define VERBOSE
// #define INTERRUPT_MODE

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
#ifdef VERBOSE2
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
#ifdef VERBOSE2
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

void *sbrk(size_t len)
{
  static unsigned long rused = 0;
  char *rd = rused + (char *)get_ddr_base();
  rused += ((len-1)|7)+1;
  return rd;
}


#define PORT 8888   //The port on which to send data
#define CHUNK_SIZE 1448

void process_ip_packet(const u_char *, int);
void print_ip_packet(const u_char * , int);
void print_tcp_packet(const u_char * , int);
void process_udp_packet(const u_char *, int, uint16_t, uint32_t, const u_char *);
void PrintData (const u_char * , int);

#define min(x,y) (x) < (y) ? (x) : (y)

int eth_discard = 0;
void process_my_packet(int size, const u_char *buffer);

static char *proto2txt(int proto)
{
  static char txt[20];
  switch(proto)
    {
    case ETH_P_IP: return "IP";
    case ETH_P_ARP: return "ARP";
    case ETH_P_IPV6: return "IP6";
    default:
      sprintf(txt, "0x%x", proto);
      return txt;
    }
}

static int copyin_pkt(void)
{
  int i, last, needed = 1;
  int rsr = eth_read(RSR_OFFSET);
  int buf = rsr & RSR_RECV_FIRST_MASK;
  int errs = 0; // eth_read(RBAD_OFFSET);
  int len = (eth_read(RPLR_OFFSET+((buf&7)<<3)) & RPLR_LENGTH_MASK) - 4;
  if ((len >= 14) && (len <= max_packet) && ((0x101<<(buf&7)) & ~errs) && !eth_discard)
    {
      int rnd, start = (RXBUFF_OFFSET>>3) + ((buf&7)<<8);
      uint64_t *alloc = rxbuf[rxhead].alloc;
      uint32_t *alloc32 = (uint32_t *)(eth_base+start);
      // Do we need to read the packet at all ??
      uint16_t rxheader = alloc32[HEADER_OFFSET >> 2];
      int proto_type = ntohs(rxheader) & 0xFFFF;
#ifdef VERBOSE
      printf("length = %d, proto = %s, (buf = %x)\n", len, proto2txt(proto_type), buf);
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
                  if (uip_ipaddr_cmp(&BUF->destipaddr, &uip_hostaddr))
                    {
                      needed = 1;
                    }
                  }
                  break;
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
                    if (dport != 1534)
                      {
                        needed = 1;
                      }
                  }
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
             if(uip_ipaddr_cmp(&BUF->dipaddr, &uip_hostaddr))
               {
                 needed = 1;
               }
            }
            break;
          case ETH_P_IPV6:
            break;
          }
        if (needed)
          {
            rnd = ((len-1|7)+1); /* round to a multiple of 8 */
            for (i = 0; i < rnd/8; i++)
              {
                alloc[i] = eth_base[start+i];
              }
#ifdef VERBOSE
            PrintData ((u_char *)alloc, len);
#endif      
            rxbuf[rxhead].len = len;
            rxhead = (rxhead + 1) % queuelen;
          }
    }
  eth_write(RSR_OFFSET, buf+1); /* acknowledge */
  return len;
}

// max size of file image is 12M
#define MAX_FILE_SIZE (CHUNK_SIZE*sizeof(uint64_t)*sizeof_maskarray)

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
}

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
  /* Check if there is Rx Data available */
  while (eth_read(RSR_OFFSET) & RSR_RECV_DONE_MASK)
    {
#ifdef VERBOSE
      printf("Ethernet interrupt\n");
#endif  
      int length = copyin_pkt();
    }
  uart_base[0x200] = 0;
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

#define rand32() ((unsigned int) rand() | ( (unsigned int) rand() << 16))
uint64_t rand64(void) { uint64_t low = rand32(), high = rand32(); return low | (high << 32); }

static uint64_t random_pkt[max_packet/sizeof(uint64_t)];

void loopback_test(int loops, int sim)
  {
    int j;
    uint64_t hi = eth_read(MACHI_OFFSET) & MACHI_MACADDR_MASK;
    eth_write(MACHI_OFFSET, MACHI_LOOPBACK_MASK|MACHI_ALLPKTS_MASK|hi);
    for (j = 1; j <= loops; j++)
      {
	enum {maxcnt=187};
	int i, waiting, actualwait, match = 0, waitcnt = 0, errcnt = 0;
	int tstcnt = 1 << j;
        int buf = (eth_read(RSR_OFFSET) & RSR_RECV_NEXT_MASK) >> 4;
        int start = RXBUFF_OFFSET + ((buf&7)<<11);
        uint64_t frq = eth_read(RBAD_OFFSET);
        
	if (tstcnt > maxcnt) tstcnt = maxcnt; /* max length packet */
	if (!sim)
          {
            printf("Selftest iteration %d, next buffer = %d, rx_start = %x\n", j, buf, start);
            printf("rx_frq = %d\n", 10*(uint32_t)(frq&0xFFFFFFFF));
            printf("tx_frq = %d\n", 10*(uint32_t)(frq>>32));
          }
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

void init_plic(void)
{
  int i;
  for (i = 1; i <= 64; i++)
    {
      plic[i] = 1;
    }
  for (i = 1; i <= 64; i++)
    {
      plic[0x800+i/32] |= 1<<(i&31);
    }
  plic[0x80000] = 0;
  for (i = 0; i < 4; i++)
    printf("%x: %x\n", i, plic[i]);
  for (i = 0x800; i < 0x804; i++)
    printf("%x: %x\n", i, plic[i]);
}

int eth_main(void) {
  int test = 0;
  int delay = 0;
  int dhcp_off_cnt = 0;
  int dhcp_ack_cnt = 0;
  uip_ipaddr_t addr;
  uint64_t lo = eth_read(MACLO_OFFSET);
  uint64_t hi = eth_read(MACHI_OFFSET) & MACHI_MACADDR_MASK;
  eth_write(MACHI_OFFSET, MACHI_IRQ_EN |MACHI_ALLPKTS_MASK |hi);
  rxbuf = (inqueue_t *)sbrk(sizeof(inqueue_t)*queuelen);
  txbuf = (outqueue_t *)sbrk(sizeof(outqueue_t)*queuelen);
  
  //  maskarray = (uint64_t *)sbrk(sizeof_maskarray);
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
  uip_ipaddr(&addr, 192,168,0,51);
  uip_sethostaddr(&addr);

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
    if (++delay == 1000000) if (get_sw() & (1<<11))
      {
        enum {max = 1280};
        int i, len;
        uint32_t peer_ip = 0;
        uint32_t srcaddr;
        uint16_t peer_port = 8888;
        u_char peer_addr[6];
        char *teststr = (char *)random_pkt;
        memset(peer_addr, -1, 6);
        memcpy(&srcaddr, &uip_hostaddr, 4);
        sprintf(teststr, "Send test packet %d\n", ++test);
        len = strlen(teststr)+1;
        for (i = len; i < max; i++) teststr[i] = i&1 ? i>>8 : i;
        delay = 0;
        udp_send(mac_addr.addr, teststr, max, PORT, peer_port, srcaddr, peer_ip, peer_addr);
        printf("%s", teststr);
      }
#ifndef INTERRUPT_MODE
    while (eth_read(RSR_OFFSET) & RSR_RECV_DONE_MASK) copyin_pkt();
#endif
    if ((txhead != txtail) && (TPLR_BUSY_MASK & ~eth_read(TPLR_OFFSET)))
      {
        uint64_t *alloc = txbuf[txtail].alloc;
        int length = txbuf[txtail].len;
        int i, rslt;
        for (i = 0; i < ((length-1|7)+1)/8; i++)
          {
            eth_write(TXBUFF_OFFSET+(i<<3), alloc[i]);
          }
        eth_write(TPLR_OFFSET,length);
#ifdef VERBOSE
        printf("TX pending\n");
        PrintData((u_char *)alloc, length);
#endif
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
                case    IPPROTO_IGMP: printf("IP Proto = IGMP\n"); break;
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
                case    IPPROTO_PIM: printf("IP Proto = PIM\n"); break;
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

void boot(uint8_t *boot_file_buf, uint32_t fsize)
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
  printf("Load %d bytes to memory address %x from boot.bin of %d bytes.\n", fsize, boot_file_buf, fsize);

  // read elf
  printf("load elf to DDR memory\n");
  if(br = load_elf(boot_file_buf, fsize))
    printf("elf read failed with code %0d", br);

  printf("Boot the loaded program...\n");

  uintptr_t mstatus = read_csr(mstatus);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPP, PRV_M);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPIE, 1);
  write_csr(mstatus, mstatus);
  write_csr(mepc, memory_base);

  printf("Goodbye, booter ...\n");
  asm volatile ("mret");
}

static uint8_t *digest = NULL;

void process_udp_packet(const u_char *data, int ulen, uint16_t peer_port, uint32_t peer_ip, const u_char *peer_addr)
{
  uint16_t idx;	
  static uint16_t maxidx;
  uint64_t siz = ((uint64_t)get_ddr_size());
  uint8_t *boot_file_buf = (uint8_t *)(get_ddr_base()) + siz - MAX_FILE_SIZE;
  uint8_t *boot_file_buf_end = (uint8_t *)(get_ddr_base()) + siz;
  uint32_t srcaddr;
  memcpy(&srcaddr, &uip_hostaddr, 4);
  if (ulen == CHUNK_SIZE+sizeof(uint16_t)+hash_length)
    {
      md5_ctx_t context;
      uint8_t *hash_value;
      char resbuf[sizeof(context.hash[0]) * 4];      
      md5_begin(&context);
      md5_hash(&context, data, CHUNK_SIZE+sizeof(uint16_t));
      md5_end(&context);
      memcpy(resbuf, context.hash, sizeof(context.hash[0]) * 4);
      if (memcmp(resbuf, data+CHUNK_SIZE+sizeof(uint16_t), hash_length))
        {
          printf("?");
        }
      else
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
                boot(boot_file_buf, maxidx*CHUNK_SIZE);
                break;
              }
            case 0xFFFE:
              {
                oldidx = 0;
                maxidx = 0;
                digest = 0;
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
                printf("Report md5 requested\n");
                if (!digest)
                  digest = hash_buf(boot_file_buf, maxidx*CHUNK_SIZE);
                udp_send(mac_addr.addr, digest, hash_length * 2 + 1, PORT, peer_port, srcaddr, peer_ip, peer_addr);
                break;
              }
            default:
              {
                uint8_t *boot_file_ptr = boot_file_buf+idx*CHUNK_SIZE;
                if (boot_file_ptr+CHUNK_SIZE < boot_file_buf_end)
                  {
                    digest = NULL;
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
    }
  else if (ulen <= 1500)
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

#define HELLO "Hello LowRISC! "__TIMESTAMP__"\n"

int main(int argc, char **argv)
{
  enum {loopback=1};
  int i, sim = argc;
  if (!sim)
    for (i = 10000000; i; i--)
      write_led(i);
  if (loopback)
    loopback_test(8, sim);
  init_plic();

  hid_send_string("lowRISC etherboot program\n=====================================\n");

  eth_main();
}
