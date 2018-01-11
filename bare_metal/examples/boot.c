// See LICENSE for license details.

#include <stdio.h>
#include "encoding.h"
#include "bits.h"
#include "diskio.h"
#include "ff.h"
#include "uart.h"
#include "elf.h"
#include "uart.h"
#include "dev_map.h"
#include "mini-printf.h"
#include "minion_lib.h"
#include "memory.h"
#include "eth.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "sdhci-minion-hash-md5.h"

FATFS FatFs;   // Work area (file system object) for logical drive
static uint8_t *digest = NULL;

// max size of file image is 10M
#define MAX_FILE_SIZE (10<<20)

// 4K size read burst
#define SD_READ_SIZE 16384

void boot(uint8_t *boot_file_buf, uint32_t fsize);

int sd_main (int sw)
{
  FIL fil;                // File object
  FRESULT fr;             // FatFs return code
  uint8_t *boot_file_buf = (uint8_t *)(get_ddr_base()) + ((uint64_t)DEV_MAP__mem__MASK + 1) - MAX_FILE_SIZE; // at the end of DDR space
  uint8_t *memory_base = (uint8_t *)(get_ddr_base());
  char nam[20];
  // Register work area to the default drive
  if(f_mount(&FatFs, "", 1)) {
    printf("Fail to mount SD driver!\n");
    return 1;
  }

  // Open a file
  printf("Switch selection = %X\n", sw);
  sprintf(nam, "boot%c%c%c%c.bin", (sw&0x8?'1':'0'), (sw&0x4?'1':'0'), (sw&0x2?'1':'0'), (sw&0x1?'1':'0'));
  printf("Load %s into memory\n", nam);
  fr = f_open(&fil, nam, FA_READ);
  if (fr) {
    printf("Failed to open %s!\n", nam);
    return (int)fr;
  }

  // Read file into memory
  uint8_t *buf = boot_file_buf;
  uint32_t fsize = 0;           // file size count
  uint32_t br;                  // Read count
  do {
    char *sum;
    fr = f_read(&fil, boot_file_buf+fsize, SD_READ_SIZE, &br);  // Read a chunk of source file
    if (!fr)
      {
	uart_send("|/-\\"[(fsize/SD_READ_SIZE)&3]);
	uart_send('\b');
	fsize += br;
      }
  } while(!(fr || br == 0));

  printf("Load %d bytes to memory address %x from boot.bin of %d bytes.\n", fsize, boot_file_buf, fil.fsize);

  // Close the file
  if(f_close(&fil)) {
    printf("fail to close file!");
    return 1;
  }
  if(f_mount(NULL, "", 1)) {         // unmount it
    printf("fail to umount disk!");
    return 1;
  }

  while (fsize & 1023)
    boot_file_buf[fsize++] = 0;
  
  boot(boot_file_buf, fsize);
}

uip_ipaddr_t uip_hostaddr, uip_draddr, uip_netmask;

const uip_ipaddr_t uip_broadcast_addr =
  { { 0xff, 0xff, 0xff, 0xff } };

const uip_ipaddr_t uip_all_zeroes_addr = { { 0x0, /* rest is 0 */ } };

uip_lladdr_t uip_lladdr;

enum {queuelen = 1024};

int rxhead, rxtail, txhead, txtail;

typedef struct inqueue_t {
  void *alloc;
  int rplr;
  int fcs;
} inqueue_t;

inqueue_t *rxbuf;

typedef struct outqueue_t {
  void *alloc;
  int len;
} outqueue_t;

outqueue_t *txbuf;

#define VERBOSEXMIT
#define UDP_DEBUG

// LowRISC Ethernet base address
static volatile unsigned int * const eth_base = (volatile unsigned int*)((uint32_t)(DEV_MAP__io_ext_eth__BASE));

static void axi_write(size_t addr, int data)
{
  if (addr < 0x2000)
    eth_base[addr >> 2] = data;
  else
    printf("axi_write(%x,%x) out of range\n", addr, data);
}

static int axi_read(size_t addr)
{
  if (addr < 0x2000)
    return eth_base[addr >> 2];
  else
    printf("axi_read(%x) out of range\n", addr);
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

static uip_eth_addr mac_addr;

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

void *sbrk(size_t len)
{
  static unsigned long rused = 0;
  char *rd = rused + (char *)get_ddr_base();
  rused += ((len-1)|7)+1;
  return rd;
}

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

#define PORT 8888   //The port on which to send data
#define CHUNK_SIZE 1024

void process_ip_packet(const u_char * , int);
void print_ip_packet(const u_char * , int);
void print_tcp_packet(const u_char * , int );
void process_udp_packet(const u_char * , int);
void PrintData (const u_char * , int);
int raw_udp_main(void *, int);

uint16_t __bswap_16(uint16_t x)
{
	return ((x << 8) & 0xff00) | ((x >> 8) & 0x00ff);
}

uint32_t __bswap_32(uint32_t x)
{
  return
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |		      \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24)) ;
}

#   define ntohl(x)     __bswap_32 (x)
#   define ntohs(x)     __bswap_16 (x)
#   define htonl(x)     __bswap_32 (x)
#   define htons(x)     __bswap_16 (x)

#define min(x,y) (x) < (y) ? (x) : (y)

int eth_discard = 0;
void process_my_packet(int size, const u_char *buffer);
void *sbrk(size_t len);

static int copyin_pkt(void)
{
  int i;
  int fcs = axi_read(RFCS_OFFSET);
  int rplr = axi_read(RPLR_OFFSET);
  int length = rplr & RPLR_LENGTH_MASK;
#ifdef VERBOSE
      printf("length = %d (rplr = %x)\n", length, rplr);
#endif      
      if ((length >= 14) && !eth_discard)
    {
      int rnd;
      uint32_t *alloc;
      rnd = ((length-1|3)+1); /* round to a multiple of 4 */
      alloc = sbrk(rnd);
      for (i = 0; i < rnd/4; i++)
        {
          alloc[i] = axi_read(RXBUFF_OFFSET+(i<<2));
        }
      rxbuf[rxhead].fcs = fcs;
      rxbuf[rxhead].rplr = rplr;
      rxbuf[rxhead].alloc = alloc;
      rxhead = (rxhead + 1) % queuelen;
    }
  axi_write(RSR_OFFSET, 0); /* acknowledge */
  return length;
}

static void lite_queue(void *buf, int length)
{
  int i, rslt;
  int rnd = ((length-1|3)+1);
  uint32_t *alloc = sbrk(rnd);
  memcpy(alloc, buf, length);
  txbuf[txhead].alloc = alloc;
  txbuf[txhead].len = length;
  txhead = (txhead+1) % queuelen;
}

// max size of file image is 10M
#define MAX_FILE_SIZE (10<<20)

// size of DDR RAM (128M for NEXYS4-DDR) 
#define DDR_SIZE 0x8000000

enum {sizeof_maskarray=MAX_FILE_SIZE/CHUNK_SIZE/8};

static int oldidx;
static uint16_t peer_port;
static u_char peer_addr[6];
static uint64_t maskarray[sizeof_maskarray/sizeof(uint64_t)];

void external_interrupt(void)
{
  int handled = 0;
#ifdef VERBOSE
  printf("Hello external interrupt! "__TIMESTAMP__"\n");
#endif
  /* Check if there is Rx Data available */
  if (axi_read(RSR_OFFSET) & RSR_RECV_DONE_MASK)
    {
#ifdef VERBOSE
      printf("Ethernet interrupt\n");
#endif  
      int length = copyin_pkt();
      handled = 1;
    }
  if (uart_check_read_irq())
    {
      int rslt = uart_read_irq();
      printf("uart interrupt read %x (%c)\n", rslt, rslt);
      handled = 1;
    }
  if (!handled)
    {
      printf("unhandled interrupt!\n");
    }
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
  
void loopback_test(int loops, int sim)
  {
    int j;
    axi_write(MACHI_OFFSET, MACHI_LOOPBACK_MASK);
    for (j = 1; j <= loops; j++)
      {
	enum {maxcnt=375};
	int i, waiting, actualwait, match = 0, waitcnt = 0;
	int tstcnt = 2 << j;
	if (tstcnt > maxcnt) tstcnt = maxcnt; /* max length packet */
	if (!sim) printf("Selftest iteration %d\n", j);
      /* bit-level digital loopback */
      axi_write(RSR_OFFSET, 0); /* clear pending receive packet, if any */
      /* random inits */
      if (!sim) for (i = 0; i < tstcnt*4; i += 4)
	{
	axi_write(TXBUFF_OFFSET+i, rand32());
	axi_write(RXBUFF_OFFSET+i, i);
	//	axi_write(AXISBUFF_OFFSET+i, i);
	}
      /* systematic inits */
      axi_write(TXBUFF_OFFSET, 0xFFFFFFFF);
      axi_write(TXBUFF_OFFSET+4, 0xFFFFFFFF);
      axi_write(TXBUFF_OFFSET+8, 0xDEADBEEF);
      axi_write(TXBUFF_OFFSET+12, 0x55555555);
      /* launch the packet */
      axi_write(TPLR_OFFSET,tstcnt*4);
      /* wait for loopback to do its work */
      do 
	{
	  waiting = (axi_read(RSR_OFFSET) == 0);
	  if (waiting) actualwait = waitcnt;
	}
      while ((waitcnt++ < tstcnt) || waiting);
      for (i = 0; i < tstcnt; i++)
	{
	  uint32_t xmit_rslt = axi_read(TXBUFF_OFFSET+(i<<2));
	  uint32_t axis_rslt = axi_read(RXBUFF_OFFSET+(i<<2));
	  if (xmit_rslt != axis_rslt)
	    {
	      if (sim)
		axi_write(MACHI_OFFSET, MACHI_LOOPBACK_MASK|MACHI_COOKED_MASK);
	      else
	        printf("Buffer offset %d: xmit=%x, axis=%x\n", i, xmit_rslt, axis_rslt);
	    }
	  else
	    ++match;
	}
      if (!sim)
	printf("Selftest matches=%d/%d, delay = %d\n", match, tstcnt, actualwait);
      }
  }

int eth_main(void) {
  uip_ipaddr_t addr;
  rxbuf = (inqueue_t *)sbrk(sizeof(inqueue_t)*queuelen);
  txbuf = (outqueue_t *)sbrk(sizeof(outqueue_t)*queuelen);
  //  maskarray = (uint64_t *)sbrk(sizeof_maskarray);
  memset(maskarray, 0, sizeof_maskarray);
  
#ifdef VERBOSE  
  printf("MAC = %x:%x\n", axi_read(MACHI_OFFSET)&MACHI_MACADDR_MASK, axi_read(MACLO_OFFSET));
  
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
  printf("IP Address:  %d.%d.%d.%d\n", uip_ipaddr_to_quad(&addr));
  uip_sethostaddr(&addr);
    
  uip_ipaddr(&addr, 255,255,255,0);
  uip_setnetmask(&addr);
  printf("Subnet Mask: %d.%d.%d.%d\n", uip_ipaddr_to_quad(&addr));
  memset(peer_addr, -1, sizeof(peer_addr));
  peer_port = PORT;
  
  printf("Enabling interrupts\n");
  old_mstatus = read_csr(mstatus);
  old_mie = read_csr(mie);
  set_csr(mstatus, MSTATUS_MIE|MSTATUS_HIE);
  set_csr(mie, ~(1 << IRQ_M_TIMER));
#if 0
  printf("Enabling UART interrupt\n");
  uart_enable_read_irq();
#endif
  do {
    if ((txhead != txtail) && (TPLR_BUSY_MASK & ~axi_read(TPLR_OFFSET)))
      {
        uint32_t *alloc = txbuf[txtail].alloc;
        int length = txbuf[txtail].len;
        int i, rslt;
#ifdef VERBOSE
        printf("TX pending\n");
#endif
        for (i = 0; i < ((length-1|3)+1)/4; i++)
          {
            axi_write(TXBUFF_OFFSET+(i<<2), alloc[i]);
          }
        axi_write(TPLR_OFFSET,length);
        txtail = (txtail + 1) % queuelen;
      }
    if (rxhead != rxtail)
      {
        uint32_t *alloc = rxbuf[rxtail].alloc;
        int rplr = rxbuf[rxtail].rplr;
        int length, xlength = rplr & RPLR_LENGTH_MASK;
        int rxheader = alloc[HEADER_OFFSET >> 2];
        int proto_type = ntohs(rxheader) & 0xFFFF;
#ifdef VERBOSE
        printf("alloc = %x\n", alloc);
        printf("rxhead = %d, rxtail = %d\n", rxhead, rxtail);
#endif
        if (rxbuf[rxtail].fcs != 0xc704dd7b)
          printf("RX FCS = %x\n", rxbuf[rxtail].fcs);
        switch (proto_type)
          {
          case ETH_P_IP:
            {
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
              } *BUF = ((struct ethip_hdr *)alloc);
              uip_ipaddr_t addr = BUF->srcipaddr;
#ifdef VERBOSE
              printf("proto = IP\n");
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
                      lite_queue(alloc, xlength+4);
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
                    //                    peer_port = ntohs(udp_hdr->uh_sport);
                    switch (dport)
                      {
                      case PORT:
                        if (memcmp(peer_addr, BUF->ethhdr.src.addr, 6))
                          {
                            memcpy(peer_addr, BUF->ethhdr.src.addr, 6);
                            printf("Peer MAC address = %02x:%02x:%02x:%02x:%02x:%02x.\n",
                                   peer_addr[0],
                                   peer_addr[1],
                                   peer_addr[2],
                                   peer_addr[3],
                                   peer_addr[4],
                                   peer_addr[5]
                                   );
                          }
                        process_udp_packet(udp_hdr->body, ulen-sizeof(struct udphdr));
                        break;
                      default:
#ifdef VERBOSE                      
                        printf("IP Proto = UDP, source port = %d, dest port = %d, length = %d\n",
                           ntohs(udp_hdr->uh_sport),
                           dport,
                           ulen);
#else
			printf("?");
#endif                      
                        break;
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
              } *BUF = ((struct arp_hdr *)alloc);
#ifdef VERBOSE
             printf("proto = ARP\n");
#endif
             if(uip_ipaddr_cmp(&BUF->dipaddr, &uip_hostaddr)) {
                int len = sizeof(struct arp_hdr);
                BUF->opcode = htons(2);
                
                memcpy(BUF->dhwaddr.addr, BUF->shwaddr.addr, 6);
                memcpy(BUF->shwaddr.addr, uip_lladdr.addr, 6);
                memcpy(BUF->ethhdr.src.addr, uip_lladdr.addr, 6);
                memcpy(BUF->ethhdr.dest.addr, BUF->dhwaddr.addr, 6);
                
                uip_ipaddr_copy(&BUF->dipaddr, &BUF->sipaddr);
                uip_ipaddr_copy(&BUF->sipaddr, &uip_hostaddr);
                
                BUF->ethhdr.type = htons(UIP_ETHTYPE_ARP);
                
#ifdef VERBOSE
                printf("sending ARP reply (length = %d)\n", len);
#endif
                lite_queue(alloc, len);
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

int just_jump (void)
{
  uint8_t *memory_base = (uint8_t *)(get_ddr_base());
  uintptr_t mstatus = read_csr(mstatus);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPP, PRV_M);
  mstatus = INSERT_FIELD(mstatus, MSTATUS_MPIE, 1);
  write_csr(mstatus, mstatus);
  write_csr(mepc, memory_base);
  asm volatile ("mret");
}

void boot(uint8_t *boot_file_buf, uint32_t fsize)
{
  uint32_t br;
  int status;
  printf("Disabling Ethernet\n");
  axi_write(MACHI_OFFSET, (axi_read(MACHI_OFFSET)&MACHI_MACADDR_MASK) | MACHI_LOOPBACK_MASK);
  eth_discard = 1;
  if (!digest)
    digest = hash_buf(boot_file_buf, fsize);
  printf("Digest of %d bytes = %s\n", fsize, digest);
  printf("Disabling interrupts, Ethernet interrupt = %d\n", axi_read(RSR_OFFSET));
  write_csr(mie, 0);
  write_csr(mstatus, old_mstatus);
  uart_disable_read_irq();

  printf("Loaded %d bytes to memory address %x from boot.bin\n", fsize, boot_file_buf, fsize);

  // read elf
  printf("load elf to DDR memory\n");
  if(br = load_elf(boot_file_buf, fsize))
    printf("elf read failed with code %0d", br);

  printf("Boot the loaded program...\n");
  just_jump();
}

void process_udp_packet(const u_char *data, int ulen)
{
  uint16_t idx;	
  static uint16_t maxidx;
  uint64_t siz = ((uint64_t)DEV_MAP__mem__MASK + 1);
  uint8_t *boot_file_buf = (uint8_t *)(get_ddr_base()) + siz - MAX_FILE_SIZE;
  uint8_t *boot_file_buf_end = (uint8_t *)(get_ddr_base()) + siz;
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
            raw_udp_main(maskarray, sizeof_maskarray);
            break;
          }
        case 0xFFFD:
          {
            printf("Report blocks requested\n");
            raw_udp_main(maskarray, sizeof_maskarray);
            break;
          }
        case 0xFFFC:
          {
            printf("Report md5 requested\n");
            if (!digest)
              digest = hash_buf(boot_file_buf, maxidx*CHUNK_SIZE);
            raw_udp_main(digest, hash_length * 2 + 1);
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
  else
    {
      printf("UDP packet length %d too short for port %d\n", ulen, PORT);
#ifdef UDP_DEBUG
      PrintData(data, ulen);
#endif      
      raw_udp_main((void *)data, ulen);
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

    // ----rawudp.c------

/*
**************************************************************************
Function: udp_sum_calc()
Description: Calculate UDP checksum
***************************************************************************
*/

uint16_t udp_sum_calc(uint16_t len_udp, uint16_t src_addr[],uint16_t dest_addr[], int padding, uint16_t buff[])
{
  int i;
  uint16_t prot_udp=17;
  uint16_t padd=0;
  uint16_t word16;
  uint32_t sum;	
	
  // Find out if the length of data is even or odd number. If odd,
  // add a padding byte = 0 at the end of packet
  if (padding&1==1){
    padd=1;
    buff[len_udp]=0;
  }
  
  //initialize sum to zero
  sum=0;
  
  // make 16 bit words out of every two adjacent 8 bit words and 
  // calculate the sum of all 16 vit words
  for (i=0;i<len_udp+padd;i=i+2){
    word16 =((buff[i]<<8)&0xFF00)+(buff[i+1]&0xFF);
    sum = sum + (unsigned long)word16;
  }	
  // add the UDP pseudo header which contains the IP source and destinationn addresses
  for (i=0;i<4;i=i+2){
    word16 =((src_addr[i]<<8)&0xFF00)+(src_addr[i+1]&0xFF);
    sum=sum+word16;	
  }
  for (i=0;i<4;i=i+2){
    word16 =((dest_addr[i]<<8)&0xFF00)+(dest_addr[i+1]&0xFF);
		sum=sum+word16; 	
  }
  // the protocol number and the length of the UDP packet
  sum = sum + prot_udp + len_udp;
  
  // keep only the last 16 bits of the 32 bit calculated sum and add the carries
  while (sum>>16)
    sum = (sum & 0xFFFF)+(sum >> 16);
  
  // Take the one's complement of sum
  sum = ~sum;

  return ((uint16_t) sum);
}

    // Source IP, source port, target IP, target port from the command line arguments

int raw_udp_main(void *msg, int payload_size)
    {
      uip_ipaddr_t src_addr, dst_addr;
      static uint8_t raw_udp[1536];
      static uint16_t idcnt = 1000;
      struct ethhdr *eth = (struct ethhdr *)raw_udp;
      struct iphdr *ip = (struct iphdr *) (raw_udp + sizeof(struct ethhdr));
      struct udphdr *udp = (struct udphdr *)
	(raw_udp + sizeof(struct ethhdr) + sizeof(struct iphdr));
      char *payload = 
	(raw_udp + sizeof(struct ethhdr) +
	 sizeof(struct iphdr) +
	 sizeof(struct udphdr));
      memcpy(payload, msg, payload_size);
    
      // The destination IP address

      uip_ipaddr(&dst_addr, 192,168,0,53);
      memcpy(&(ip->daddr), &dst_addr, 4);

      // Fill in the ethernet header
      memcpy(eth->h_dest, peer_addr, 6); // dest address
      memcpy(eth->h_source, mac_addr.addr, 6); // our address
      eth->h_proto = 8;
      // Fabricate the IP header or we can use the
      // standard header structures but assign our own values.

    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0; // Low delay
    ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + payload_size);
    ip->id = htons(idcnt);
    idcnt++;
    ip->frag_off = htons(0x4000);
    ip->ttl = 64; // hops
    ip->protocol = IPPROTO_UDP; // UDP

    // Source IP address, can use spoofed address here!!!

    uip_ipaddr(&src_addr, 192,168,0,51);
    memcpy(&(ip->saddr), &src_addr, 4);

    // Fabricate the UDP header. Source port number, redundant

    udp->uh_sport = htons(PORT);

    // Destination port number

    udp->uh_dport = htons(peer_port);
    udp->uh_ulen = htons(sizeof(struct udphdr) + payload_size);
    udp->uh_sum = udp_sum_calc(payload_size, (uint16_t *)&(ip->saddr), (uint16_t *)&(ip->daddr), payload_size & 1 ? 0 : 1, (uint16_t *)payload);
    udp->uh_sum = 0;

    // Calculate the checksum for integrity

    ip->check = csum(raw_udp, sizeof(struct iphdr) + sizeof(struct udphdr) + payload_size);
    ip->check = 0; // hack
    
    lite_queue(raw_udp, sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr) + payload_size);
    return 0;

    }

#define HELLO "Hello LowRISC! "__TIMESTAMP__"\n"

int main()
{
  uint32_t macaddr_lo, macaddr_hi;
  int sw = sd_resp(31);
  uart_init();
  loopback_test(8, (sw & 0xF) == 0xF);
  board_mmc_power_init();  

  uart_send_string("lowRISC boot program\n=====================================\n");
  mac_addr.addr[0] = (uint8_t)0xEE;
  mac_addr.addr[1] = (uint8_t)0xE1;
  mac_addr.addr[2] = (uint8_t)0xE2;
  mac_addr.addr[3] = (uint8_t)0xE3;
  mac_addr.addr[4] = (uint8_t)0xE4;
  mac_addr.addr[5] = (uint8_t)(0xE0|(sw >> 12));

  memcpy (&macaddr_lo, mac_addr.addr+2, sizeof(uint32_t));
  memcpy (&macaddr_hi, mac_addr.addr+0, sizeof(uint16_t));
  axi_write(MACLO_OFFSET, htonl(macaddr_lo));
  axi_write(MACHI_OFFSET, MACHI_IRQ_EN|htons(macaddr_hi));
  if (sw & 1)
      {
        uart_send_string(HELLO"Jumping to DRAM because SW0 is high ..\r\n");
        just_jump();
      }
  if (sw & 2)
      {
        uart_send_string(HELLO"Booting from FLASH because SW1 is high ..\r\n");
        sd_main(sw>>3);
      }
  if (sw & 4)
      {
        uart_send_string(HELLO"Booting from Ethernet because SW2 is high ..\r\n");
        eth_main();
      }
  uart_send_string(HELLO"Turn on SW0 for trace debugger loading, SW1 for SD-card loading, or SW2 for Ethernet loading\r\n");
  uart_recv();
}
