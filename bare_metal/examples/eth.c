/*
 * The Minimal snprintf() implementation
 *
 * Copyright (c) 2013 Michal Ludvig <michal@logix.cz>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the auhor nor the names of its contributors
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h> // for exit()
#include <string.h> // for memset
#include <string.h>
#include <stdarg.h>

int mini_vsnprintf(char* buffer, unsigned int buffer_len, const char *fmt, va_list va);
int mini_snprintf(char* buffer, unsigned int buffer_len, const char *fmt, ...);
int mini_printf(const char *fmt, ...);

#define vsnprintf mini_vsnprintf
#define snprintf mini_snprintf
#define printf mini_printf

// A hello eth program
#include "encoding.h"
#include "bits.h"
#include "elf.h"
#include "uart.h"
#include "dev_map.h"
#include "eth.h"
#include "net/ipv4/uip_arp.h"

volatile uint64_t * get_ddr_base() {
  return (uint64_t *)(DEV_MAP__mem__BASE);
}

static volatile unsigned int * const eth_base = (volatile unsigned int*)ETH_BASE;

static void axi_write(int pingpong, size_t addr, int data, int strb)
{
  eth_base[((pingpong&1 ? XEL_BUFFER_OFFSET : 0) | addr) >> 2] = data;
}

static int axi_read(int pingpong, size_t addr)
{
  return eth_base[((pingpong&1 ? XEL_BUFFER_OFFSET : 0) | addr) >> 2];
}

/* Register offsets for the EmacLite Core */
#define XEL_TXBUFF_OFFSET	0x0		/* Transmit Buffer */
#define XEL_MDIOADDR_OFFSET	0x07E4		/* MDIO Address Register */
#define XEL_MDIOWR_OFFSET	0x07E8		/* MDIO Write Data Register */
#define XEL_MDIORD_OFFSET	0x07EC		/* MDIO Read Data Register */
#define XEL_MDIOCTRL_OFFSET	0x07F0		/* MDIO Control Register */
#define XEL_GIER_OFFSET		0x07F8		/* GIE Register */
#define XEL_TSR_OFFSET		0x07FC		/* Tx status */
#define XEL_TPLR_OFFSET		0x07F4		/* Tx packet length */

#define XEL_RXBUFF_OFFSET	0x1000		/* Receive Buffer */
#define XEL_RPLR_OFFSET		0x100C		/* Rx packet length */
#define XEL_RSR_OFFSET		0x17FC		/* Rx status */

#define XEL_BUFFER_OFFSET	0x0800		/* Next Tx/Rx buffer's offset */

/* MDIO Address Register Bit Masks */
#define XEL_MDIOADDR_REGADR_MASK  0x0000001F	/* Register Address */
#define XEL_MDIOADDR_PHYADR_MASK  0x000003E0	/* PHY Address */
#define XEL_MDIOADDR_PHYADR_SHIFT 5
#define XEL_MDIOADDR_OP_MASK	  0x00000400	/* RD/WR Operation */

/* MDIO Write Data Register Bit Masks */
#define XEL_MDIOWR_WRDATA_MASK	  0x0000FFFF	/* Data to be Written */

/* MDIO Read Data Register Bit Masks */
#define XEL_MDIORD_RDDATA_MASK	  0x0000FFFF	/* Data to be Read */

/* MDIO Control Register Bit Masks */
#define XEL_MDIOCTRL_MDIOSTS_MASK 0x00000001	/* MDIO Status Mask */
#define XEL_MDIOCTRL_MDIOEN_MASK  0x00000008	/* MDIO Enable */

/* Global Interrupt Enable Register (GIER) Bit Masks */
#define XEL_GIER_GIE_MASK	0x80000000	/* Global Enable */

/* Transmit Status Register (TSR) Bit Masks */
#define XEL_TSR_XMIT_BUSY_MASK	 0x00000001	/* Tx complete */
#define XEL_TSR_PROGRAM_MASK	 0x00000002	/* Program the MAC address */
#define XEL_TSR_XMIT_IE_MASK	 0x00000008	/* Tx interrupt enable bit */
#define XEL_TSR_XMIT_ACTIVE_MASK 0x80000000	/* Buffer is active, SW bit
						 * only. This is not documented
						 * in the HW spec */

/* Define for programming the MAC address into the EmacLite */
#define XEL_TSR_PROG_MAC_ADDR	(XEL_TSR_XMIT_BUSY_MASK | XEL_TSR_PROGRAM_MASK)

/* Receive Status Register (RSR) */
#define XEL_RSR_RECV_DONE_MASK	0x00000001	/* Rx complete */
#define XEL_RSR_RECV_IE_MASK	0x00000008	/* Rx interrupt enable bit */

/* Transmit Packet Length Register (TPLR) */
#define XEL_TPLR_LENGTH_MASK	0x0000FFFF	/* Tx packet length */

/* Receive Packet Length Register (RPLR) */
#define XEL_RPLR_LENGTH_MASK	0x0000FFFF	/* Rx packet length */

#define XEL_HEADER_OFFSET	12		/* Offset to length field */
#define XEL_HEADER_SHIFT	16		/* Shift value for length */

/* General Ethernet Definitions */
#define XEL_ARP_PACKET_SIZE		28	/* Max ARP packet size */
#define XEL_HEADER_IP_LENGTH_OFFSET	16	/* IP Length Offset */

#define ETH_DATA_LEN	1500		/* Max. octets in payload	 */
#define ETH_P_IP	0x0800		/* Internet Protocol packet	*/
#define ETH_HLEN	14		/* Total octets in header.	 */
#define ETH_FCS_LEN	4		/* Octets in the FCS		 */
#define ETH_P_ARP	0x0806		/* Address Resolution packet	*/
#define ETH_FRAME_LEN	1514		/* Max. octets in frame sans FCS */

static void eth_poll(int pp);
static uip_eth_addr mac_addr;

typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;
typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;
typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;
typedef long int __quad_t;
typedef unsigned long int __u_quad_t;
typedef unsigned int __uid_t;
typedef unsigned int __gid_t;
typedef unsigned long int __ino_t;
typedef unsigned long int __ino64_t;
typedef unsigned int __mode_t;
typedef long int __off_t;
typedef long int __off64_t;
typedef int __pid_t;
typedef struct { int __val[2]; } __fsid_t;
typedef long int __clock_t;
typedef unsigned long int __rlim_t;
typedef unsigned long int __rlim64_t;
typedef unsigned int __id_t;
typedef long int __time_t;
typedef unsigned int __useconds_t;
typedef long int __suseconds_t;
typedef int __daddr_t;
typedef long int __blksize_t;
typedef long int __blkcnt_t;
typedef long int __blkcnt64_t;
typedef unsigned long int __fsblkcnt_t;
typedef unsigned long int __fsblkcnt64_t;
typedef unsigned long int __fsfilcnt_t;
typedef unsigned long int __fsfilcnt64_t;
typedef long int __fsword_t;
typedef long int __ssize_t;
typedef long int __syscall_slong_t;
typedef unsigned long int __syscall_ulong_t;
typedef __quad_t *__qaddr_t;
typedef char *__caddr_t;
typedef long int __intptr_t;
typedef unsigned int __socklen_t;
typedef __u_char u_char;
typedef __u_short u_short;
typedef __u_int u_int;
typedef __u_long u_long;
typedef __quad_t quad_t;
typedef __u_quad_t u_quad_t;
typedef __fsid_t fsid_t;
typedef __ino_t ino_t;
typedef __gid_t gid_t;
typedef __mode_t mode_t;
typedef __nlink_t nlink_t;
typedef __uid_t uid_t;
typedef __off_t off_t;
typedef __pid_t pid_t;
typedef __id_t id_t;
typedef __ssize_t ssize_t;
typedef __caddr_t caddr_t;
typedef __time_t time_t;


typedef long unsigned int size_t;
typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;
typedef int int8_t __attribute__ ((__mode__ (__QI__)));
typedef int int16_t __attribute__ ((__mode__ (__HI__)));
typedef int int32_t __attribute__ ((__mode__ (__SI__)));
typedef int int64_t __attribute__ ((__mode__ (__DI__)));
typedef unsigned int u_int8_t __attribute__ ((__mode__ (__QI__)));
typedef unsigned int u_int16_t __attribute__ ((__mode__ (__HI__)));
typedef unsigned int u_int32_t __attribute__ ((__mode__ (__SI__)));
typedef unsigned int u_int64_t __attribute__ ((__mode__ (__DI__)));
typedef int register_t __attribute__ ((__mode__ (__word__)));
typedef int __sig_atomic_t;
typedef struct
  {
    unsigned long int __val[(1024 / (8 * sizeof (unsigned long int)))];
  } __sigset_t;
typedef __sigset_t sigset_t;
struct timeval
  {
    __time_t tv_sec;
    __suseconds_t tv_usec;
  };
typedef __suseconds_t suseconds_t;
typedef long int __fd_mask;
typedef __fd_mask fd_mask;

extern int select (int __nfds, fd_set *__restrict __readfds,
     fd_set *__restrict __writefds,
     fd_set *__restrict __exceptfds,
     struct timeval *__restrict __timeout);
extern int pselect (int __nfds, fd_set *__restrict __readfds,
      fd_set *__restrict __writefds,
      fd_set *__restrict __exceptfds,
      const struct timespec *__restrict __timeout,
      const __sigset_t *__restrict __sigmask);


__extension__
extern unsigned int gnu_dev_major (unsigned long long int __dev)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
__extension__
extern unsigned int gnu_dev_minor (unsigned long long int __dev)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
__extension__
extern unsigned long long int gnu_dev_makedev (unsigned int __major,
            unsigned int __minor)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));

typedef __blkcnt_t blkcnt_t;
typedef __fsblkcnt_t fsblkcnt_t;
typedef __fsfilcnt_t fsfilcnt_t;
typedef unsigned long int pthread_t;
union pthread_attr_t
{
  char __size[56];
  long int __align;
};
typedef union pthread_attr_t pthread_attr_t;
typedef struct __pthread_internal_list
{
  struct __pthread_internal_list *__prev;
  struct __pthread_internal_list *__next;
} __pthread_list_t;
typedef union
{
  struct __pthread_mutex_s
  {
    int __lock;
    unsigned int __count;
    int __owner;
    unsigned int __nusers;
    int __kind;
    short __spins;
    short __elision;
    __pthread_list_t __list;
  } __data;
  char __size[40];
  long int __align;
} pthread_mutex_t;
typedef union
{
  char __size[4];
  int __align;
} pthread_mutexattr_t;
typedef union
{
  struct
  {
    int __lock;
    unsigned int __futex;
    __extension__ unsigned long long int __total_seq;
    __extension__ unsigned long long int __wakeup_seq;
    __extension__ unsigned long long int __woken_seq;
    void *__mutex;
    unsigned int __nwaiters;
    unsigned int __broadcast_seq;
  } __data;
  char __size[48];
  __extension__ long long int __align;
} pthread_cond_t;
typedef union
{
  char __size[4];
  int __align;
} pthread_condattr_t;
typedef unsigned int pthread_key_t;
typedef int pthread_once_t;
typedef union
{
  struct
  {
    int __lock;
    unsigned int __nr_readers;
    unsigned int __readers_wakeup;
    unsigned int __writer_wakeup;
    unsigned int __nr_readers_queued;
    unsigned int __nr_writers_queued;
    int __writer;
    int __shared;
    signed char __rwelision;
    unsigned char __pad1[7];
    unsigned long int __pad2;
    unsigned int __flags;
  } __data;
  char __size[56];
  long int __align;
} pthread_rwlock_t;
typedef union
{
  char __size[8];
  long int __align;
} pthread_rwlockattr_t;
typedef volatile int pthread_spinlock_t;
typedef union
{
  char __size[32];
  long int __align;
} pthread_barrier_t;
typedef union
{
  char __size[4];
  int __align;
} pthread_barrierattr_t;


struct iovec
  {
    void *iov_base;
    size_t iov_len;
  };
extern ssize_t readv (int __fd, const struct iovec *__iovec, int __count)
  ;
extern ssize_t writev (int __fd, const struct iovec *__iovec, int __count)
  ;
extern ssize_t preadv (int __fd, const struct iovec *__iovec, int __count,
         __off_t __offset) ;
extern ssize_t pwritev (int __fd, const struct iovec *__iovec, int __count,
   __off_t __offset) ;

typedef __socklen_t socklen_t;
enum __socket_type
{
  SOCK_STREAM = 1,
  SOCK_DGRAM = 2,
  SOCK_RAW = 3,
  SOCK_RDM = 4,
  SOCK_SEQPACKET = 5,
  SOCK_DCCP = 6,
  SOCK_PACKET = 10,
  SOCK_CLOEXEC = 02000000,
  SOCK_NONBLOCK = 00004000
};
typedef unsigned short int sa_family_t;
struct sockaddr
  {
    sa_family_t sa_family;
    char sa_data[14];
  };
struct sockaddr_storage
  {
    sa_family_t ss_family;
    char __ss_padding[(128 - (sizeof (unsigned short int)) - sizeof (unsigned long int))];
    unsigned long int __ss_align;
  };
enum
  {
    MSG_OOB = 0x01,
    MSG_PEEK = 0x02,
    MSG_DONTROUTE = 0x04,
    MSG_CTRUNC = 0x08,
    MSG_PROXY = 0x10,
    MSG_TRUNC = 0x20,
    MSG_DONTWAIT = 0x40,
    MSG_EOR = 0x80,
    MSG_WAITALL = 0x100,
    MSG_FIN = 0x200,
    MSG_SYN = 0x400,
    MSG_CONFIRM = 0x800,
    MSG_RST = 0x1000,
    MSG_ERRQUEUE = 0x2000,
    MSG_NOSIGNAL = 0x4000,
    MSG_MORE = 0x8000,
    MSG_WAITFORONE = 0x10000,
    MSG_FASTOPEN = 0x20000000,
    MSG_CMSG_CLOEXEC = 0x40000000
  };
struct msghdr
  {
    void *msg_name;
    socklen_t msg_namelen;
    struct iovec *msg_iov;
    size_t msg_iovlen;
    void *msg_control;
    size_t msg_controllen;
    int msg_flags;
  };
struct cmsghdr
  {
    size_t cmsg_len;
    int cmsg_level;
    int cmsg_type;
    __extension__ unsigned char __cmsg_data [];
  };
extern struct cmsghdr *__cmsg_nxthdr (struct msghdr *__mhdr,
          struct cmsghdr *__cmsg) __attribute__ ((__nothrow__ , __leaf__));
enum
  {
    SCM_RIGHTS = 0x01
  };
struct linger
  {
    int l_onoff;
    int l_linger;
  };
struct osockaddr
  {
    unsigned short int sa_family;
    unsigned char sa_data[14];
  };
enum
{
  SHUT_RD = 0,
  SHUT_WR,
  SHUT_RDWR
};
extern int socket (int __domain, int __type, int __protocol) __attribute__ ((__nothrow__ , __leaf__));
extern int socketpair (int __domain, int __type, int __protocol,
         int __fds[2]) __attribute__ ((__nothrow__ , __leaf__));
extern int bind (int __fd, const struct sockaddr * __addr, socklen_t __len)
     __attribute__ ((__nothrow__ , __leaf__));
extern int getsockname (int __fd, struct sockaddr *__restrict __addr,
   socklen_t *__restrict __len) __attribute__ ((__nothrow__ , __leaf__));
extern int connect (int __fd, const struct sockaddr * __addr, socklen_t __len);
extern int getpeername (int __fd, struct sockaddr *__restrict __addr,
   socklen_t *__restrict __len) __attribute__ ((__nothrow__ , __leaf__));
extern ssize_t send (int __fd, const void *__buf, size_t __n, int __flags);
extern ssize_t recv (int __fd, void *__buf, size_t __n, int __flags);
extern ssize_t sendto (int __fd, const void *__buf, size_t __n,
         int __flags, const struct sockaddr * __addr,
         socklen_t __addr_len);
extern ssize_t recvfrom (int __fd, void *__restrict __buf, size_t __n,
    int __flags, struct sockaddr *__restrict __addr,
    socklen_t *__restrict __addr_len);
extern ssize_t sendmsg (int __fd, const struct msghdr *__message,
   int __flags);
extern ssize_t recvmsg (int __fd, struct msghdr *__message, int __flags);
extern int getsockopt (int __fd, int __level, int __optname,
         void *__restrict __optval,
         socklen_t *__restrict __optlen) __attribute__ ((__nothrow__ , __leaf__));
extern int setsockopt (int __fd, int __level, int __optname,
         const void *__optval, socklen_t __optlen) __attribute__ ((__nothrow__ , __leaf__));
extern int listen (int __fd, int __n) __attribute__ ((__nothrow__ , __leaf__));
extern int accept (int __fd, struct sockaddr *__restrict __addr,
     socklen_t *__restrict __addr_len);
extern int shutdown (int __fd, int __how) __attribute__ ((__nothrow__ , __leaf__));
extern int sockatmark (int __fd) __attribute__ ((__nothrow__ , __leaf__));
extern int isfdtype (int __fd, int __fdtype) __attribute__ ((__nothrow__ , __leaf__));

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long int uint64_t;
typedef signed char int_least8_t;
typedef short int int_least16_t;
typedef int int_least32_t;
typedef long int int_least64_t;
typedef unsigned char uint_least8_t;
typedef unsigned short int uint_least16_t;
typedef unsigned int uint_least32_t;
typedef unsigned long int uint_least64_t;
typedef long int intptr_t;
typedef unsigned long int uintptr_t;
typedef long int intmax_t;
typedef unsigned long int uintmax_t;

typedef uint32_t in_addr_t;
struct in_addr
  {
    in_addr_t s_addr;
  };
struct ip_opts
  {
    struct in_addr ip_dst;
    char ip_opts[40];
  };
struct ip_mreqn
  {
    struct in_addr imr_multiaddr;
    struct in_addr imr_address;
    int imr_ifindex;
  };
struct in_pktinfo
  {
    int ipi_ifindex;
    struct in_addr ipi_spec_dst;
    struct in_addr ipi_addr;
  };
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
enum
  {
    IPPROTO_HOPOPTS = 0,
    IPPROTO_ROUTING = 43,
    IPPROTO_FRAGMENT = 44,
    IPPROTO_ICMPV6 = 58,
    IPPROTO_NONE = 59,
    IPPROTO_DSTOPTS = 60,
    IPPROTO_MH = 135
  };
typedef uint16_t in_port_t;
enum
  {
    IPPORT_ECHO = 7,
    IPPORT_DISCARD = 9,
    IPPORT_SYSTAT = 11,
    IPPORT_DAYTIME = 13,
    IPPORT_NETSTAT = 15,
    IPPORT_FTP = 21,
    IPPORT_TELNET = 23,
    IPPORT_SMTP = 25,
    IPPORT_TIMESERVER = 37,
    IPPORT_NAMESERVER = 42,
    IPPORT_WHOIS = 43,
    IPPORT_MTP = 57,
    IPPORT_TFTP = 69,
    IPPORT_RJE = 77,
    IPPORT_FINGER = 79,
    IPPORT_TTYLINK = 87,
    IPPORT_SUPDUP = 95,
    IPPORT_EXECSERVER = 512,
    IPPORT_LOGINSERVER = 513,
    IPPORT_CMDSERVER = 514,
    IPPORT_EFSSERVER = 520,
    IPPORT_BIFFUDP = 512,
    IPPORT_WHOSERVER = 513,
    IPPORT_ROUTESERVER = 520,
    IPPORT_RESERVED = 1024,
    IPPORT_USERRESERVED = 5000
  };
struct in6_addr
  {
    union
      {
 uint8_t __u6_addr8[16];
 uint16_t __u6_addr16[8];
 uint32_t __u6_addr32[4];
      } __in6_u;
  };
extern const struct in6_addr in6addr_any;
extern const struct in6_addr in6addr_loopback;
struct sockaddr_in
  {
    sa_family_t sin_family;
    in_port_t sin_port;
    struct in_addr sin_addr;
    unsigned char sin_zero[sizeof (struct sockaddr) -
      (sizeof (unsigned short int)) -
      sizeof (in_port_t) -
      sizeof (struct in_addr)];
  };
struct sockaddr_in6
  {
    sa_family_t sin6_family;
    in_port_t sin6_port;
    uint32_t sin6_flowinfo;
    struct in6_addr sin6_addr;
    uint32_t sin6_scope_id;
  };
struct ip_mreq
  {
    struct in_addr imr_multiaddr;
    struct in_addr imr_interface;
  };
struct ip_mreq_source
  {
    struct in_addr imr_multiaddr;
    struct in_addr imr_interface;
    struct in_addr imr_sourceaddr;
  };
struct ipv6_mreq
  {
    struct in6_addr ipv6mr_multiaddr;
    unsigned int ipv6mr_interface;
  };
struct group_req
  {
    uint32_t gr_interface;
    struct sockaddr_storage gr_group;
  };
struct group_source_req
  {
    uint32_t gsr_interface;
    struct sockaddr_storage gsr_group;
    struct sockaddr_storage gsr_source;
  };
struct ip_msfilter
  {
    struct in_addr imsf_multiaddr;
    struct in_addr imsf_interface;
    uint32_t imsf_fmode;
    uint32_t imsf_numsrc;
    struct in_addr imsf_slist[1];
  };
struct group_filter
  {
    uint32_t gf_interface;
    struct sockaddr_storage gf_group;
    uint32_t gf_fmode;
    uint32_t gf_numsrc;
    struct sockaddr_storage gf_slist[1];
};
extern uint32_t ntohl (uint32_t __netlong) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern uint16_t ntohs (uint16_t __netshort)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern uint32_t htonl (uint32_t __hostlong)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern uint16_t htons (uint16_t __hostshort)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern int bindresvport (int __sockfd, struct sockaddr_in *__sock_in) __attribute__ ((__nothrow__ , __leaf__));
extern int bindresvport6 (int __sockfd, struct sockaddr_in6 *__sock_in)
     __attribute__ ((__nothrow__ , __leaf__));

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
#define UDPBUF ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

void process_my_packet(int pp, int size, const u_char *buffer);

static void copyin_pkt(int pp, int length)
{
  int i;
  for (i = 0; i < ((length-1|3)+1)/4; i++)
	  {
            ((uint32_t *)uip_buf)[i] = axi_read(pp, XEL_RXBUFF_OFFSET+(i<<2));
	  }
  uip_len = length;
}

static void lite_copyout(int pp, int length)
{
  int i, rslt;
  for (i = 0; i < ((length-1|3)+1)/4; i++)
	  {
	    axi_write(pp, XEL_TXBUFF_OFFSET+(i<<2), ((uint32_t *)uip_buf)[i], 0xf);
 	  }
}

static void lite_xmit(int pp, int length)
{
  int rslt;
  axi_write(pp, XEL_GIER_OFFSET,XEL_GIER_GIE_MASK,0xf);
  axi_write(pp, XEL_TPLR_OFFSET,length,0xf);
  axi_write(pp, XEL_TSR_OFFSET,XEL_TSR_XMIT_IE_MASK|XEL_TSR_XMIT_BUSY_MASK,0xf);
 do
   {
     rslt = axi_read(pp, XEL_TSR_OFFSET);
#ifdef VERBOSE
     printf("TX Status = %x\n", rslt);
#endif
   }
 while (rslt == (XEL_TSR_XMIT_IE_MASK|XEL_TSR_XMIT_BUSY_MASK));
 rslt &= ~XEL_TSR_XMIT_ACTIVE_MASK;
 axi_write(pp, XEL_TSR_OFFSET, rslt, 0xf);
}

static void eth_dummy(int pp)
{
  axi_write(pp, 0x00000000,0xffffffff,0xf);
  axi_write(pp, 0x00000004,0xffffffff,0xf);
  axi_write(pp, 0x00000008,0xffffffff,0xf);
  axi_write(pp, 0x0000000c,0x11111111,0xf);
  axi_write(pp, 0x00000010,0x22222222,0xf);
  axi_write(pp, 0x00000014,0x33333333,0xf);
  axi_read(pp, 0x0000001c);
  axi_write(pp, 0x00000020,0x55555555,0xf);
  lite_xmit(pp, 0x14);
}

static void eth_poll(int pp)
{
  int i, rslt, lenmask, proto_type, length;
  lenmask = axi_read(pp, XEL_RSR_OFFSET);
  //printf("RX Status = %x\n", lenmask);
  /* Check if there is Rx Data available */
  if (lenmask & XEL_RSR_RECV_DONE_MASK)
    {
      int header = axi_read(pp, XEL_HEADER_OFFSET + XEL_RXBUFF_OFFSET);
#ifdef VERBOSE
      printf("header = %x\n", header);
#endif
      /* Get the protocol type of the ethernet frame that arrived */
      proto_type = ntohs(header) & XEL_RPLR_LENGTH_MASK;
      
#ifdef VERBOSE
      printf("proto_type = %x\n", proto_type);
#endif
      /* Check if received ethernet frame is a raw ethernet frame
       * or an IP packet or an ARP packet */
      if (proto_type > ETH_DATA_LEN) {
        
        if (proto_type == ETH_P_IP) {
#ifdef VERBOSE
          printf("proto = IP\n");
#endif
          length = ((ntohl(axi_read(pp, 
                                    XEL_HEADER_IP_LENGTH_OFFSET +
                                    XEL_RXBUFF_OFFSET)) >>
                     XEL_HEADER_SHIFT) &
                    XEL_RPLR_LENGTH_MASK);
          length = min(length, ETH_DATA_LEN);
          length += ETH_HLEN + ETH_FCS_LEN;
          copyin_pkt(pp, length);
          process_my_packet(pp, length, uip_buf);
          
        } else if (proto_type == ETH_P_ARP)
          {
            printf("proto = ARP\n");
            length = XEL_ARP_PACKET_SIZE + ETH_HLEN + ETH_FCS_LEN;
            copyin_pkt(pp, length);
            uip_arp_arpin();
            /* If the above function invocation resulted in data that
               should be sent out on the network, the global variable
               uip_len is set to a value > 0. */
            if(uip_len > 0) {
              printf("sending ARP reply (length = %d)\n", uip_len);
              lite_copyout(pp, uip_len);
              lite_xmit(pp, uip_len);
            }
          }
        else
          {
            /* Field contains type other than IP or ARP, use max
             * frame size and let user parse it */
            length = ETH_FRAME_LEN + ETH_FCS_LEN;
            copyin_pkt(pp, length);
            process_my_packet(pp, length, uip_buf);
          }
      } else
        {
          printf("proto = raw ethernet frame\n");
          /* Use the length in the frame, plus the header and trailer */
          length = proto_type + ETH_HLEN + ETH_FCS_LEN;
          copyin_pkt(pp, length);
          process_my_packet(pp, length, uip_buf);
        }
#ifdef VERBOSE
      printf("length = %d\n", length);
#endif
      axi_write(pp, XEL_RSR_OFFSET, lenmask & ~XEL_RSR_RECV_DONE_MASK, 0xf);
    }
}

extern in_addr_t inet_addr (const char *__cp) __attribute__ ((__nothrow__ , __leaf__));
extern in_addr_t inet_lnaof (struct in_addr __in) __attribute__ ((__nothrow__ , __leaf__));
extern struct in_addr inet_makeaddr (in_addr_t __net, in_addr_t __host)
     __attribute__ ((__nothrow__ , __leaf__));
extern in_addr_t inet_netof (struct in_addr __in) __attribute__ ((__nothrow__ , __leaf__));
extern in_addr_t inet_network (const char *__cp) __attribute__ ((__nothrow__ , __leaf__));
extern int inet_pton (int __af, const char *__restrict __cp,
        void *__restrict __buf) __attribute__ ((__nothrow__ , __leaf__));
extern const char *inet_ntop (int __af, const void *__restrict __cp,
         char *__restrict __buf, socklen_t __len)
     __attribute__ ((__nothrow__ , __leaf__));
extern int inet_aton (const char *__cp, struct in_addr *__inp) __attribute__ ((__nothrow__ , __leaf__));
extern char *inet_neta (in_addr_t __net, char *__buf, size_t __len) __attribute__ ((__nothrow__ , __leaf__));
extern char *inet_net_ntop (int __af, const void *__cp, int __bits,
       char *__buf, size_t __len) __attribute__ ((__nothrow__ , __leaf__));
extern int inet_net_pton (int __af, const char *__cp,
     void *__buf, size_t __len) __attribute__ ((__nothrow__ , __leaf__));
extern unsigned int inet_nsap_addr (const char *__cp,
        unsigned char *__buf, int __len) __attribute__ ((__nothrow__ , __leaf__));
extern char *inet_nsap_ntoa (int __len, const unsigned char *__cp,
        char *__buf) __attribute__ ((__nothrow__ , __leaf__));

typedef __signed__ char __s8;
typedef unsigned char __u8;
typedef __signed__ short __s16;
typedef unsigned short __u16;
typedef __signed__ int __s32;
typedef unsigned int __u32;
__extension__ typedef __signed__ long long __s64;
__extension__ typedef unsigned long long __u64;
typedef struct {
 unsigned long fds_bits[1024 / (8 * sizeof(long))];
} __kernel_fd_set;
typedef void (*__kernel_sighandler_t)(int);
typedef int __kernel_key_t;
typedef int __kernel_mqd_t;
typedef long __kernel_long_t;
typedef unsigned long __kernel_ulong_t;
typedef __kernel_ulong_t __kernel_ino_t;
typedef unsigned int __kernel_mode_t;
typedef int __kernel_pid_t;
typedef int __kernel_ipc_pid_t;
typedef unsigned int __kernel_uid_t;
typedef unsigned int __kernel_gid_t;
typedef __kernel_long_t __kernel_suseconds_t;
typedef unsigned int __kernel_uid32_t;
typedef unsigned int __kernel_gid32_t;
typedef __kernel_ulong_t __kernel_size_t;
typedef __kernel_long_t __kernel_ssize_t;
typedef __kernel_long_t __kernel_ptrdiff_t;
typedef struct {
 int val[2];
} __kernel_fsid_t;
typedef __kernel_long_t __kernel_off_t;
typedef long long __kernel_loff_t;
typedef __kernel_long_t __kernel_time_t;
typedef __kernel_long_t __kernel_clock_t;
typedef char * __kernel_caddr_t;
typedef unsigned short __kernel_uid16_t;
typedef unsigned short __kernel_gid16_t;
typedef __u16 __le16;
typedef __u16 __be16;
typedef __u32 __le32;
typedef __u32 __be32;
typedef __u64 __le64;
typedef __u64 __be64;
typedef __u16 __sum16;
typedef __u32 __wsum;
struct ethhdr {
 unsigned char h_dest[6];
 unsigned char h_source[6];
 __be16 h_proto;
} __attribute__((packed));

struct ether_addr
{
  u_int8_t ether_addr_octet[6];
} __attribute__ ((__packed__));
struct ether_header
{
  u_int8_t ether_dhost[6];
  u_int8_t ether_shost[6];
  u_int16_t ether_type;
} __attribute__ ((__packed__));


struct icmphdr
{
  u_int8_t type;
  u_int8_t code;
  u_int16_t checksum;
  union
  {
    struct
    {
      u_int16_t id;
      u_int16_t sequence;
    } echo;
    u_int32_t gateway;
    struct
    {
      u_int16_t __glibc_reserved;
      u_int16_t mtu;
    } frag;
  } un;
};

struct timestamp
  {
    u_int8_t len;
    u_int8_t ptr;
    unsigned int flags:4;
    unsigned int overflow:4;
    u_int32_t data[9];
  };
struct iphdr
  {
    unsigned int ihl:4;
    unsigned int version:4;
    u_int8_t tos;
    u_int16_t tot_len;
    u_int16_t id;
    u_int16_t frag_off;
    u_int8_t ttl;
    u_int8_t protocol;
    u_int16_t check;
    u_int32_t saddr;
    u_int32_t daddr;
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
struct ip_timestamp
  {
    u_int8_t ipt_code;
    u_int8_t ipt_len;
    u_int8_t ipt_ptr;
    unsigned int ipt_flg:4;
    unsigned int ipt_oflw:4;
    u_int32_t data[9];
  };

struct icmp_ra_addr
{
  u_int32_t ira_addr;
  u_int32_t ira_preference;
};
struct icmp
{
  u_int8_t icmp_type;
  u_int8_t icmp_code;
  u_int16_t icmp_cksum;
  union
  {
    u_char ih_pptr;
    struct in_addr ih_gwaddr;
    struct ih_idseq
    {
      u_int16_t icd_id;
      u_int16_t icd_seq;
    } ih_idseq;
    u_int32_t ih_void;
    struct ih_pmtu
    {
      u_int16_t ipm_void;
      u_int16_t ipm_nextmtu;
    } ih_pmtu;
    struct ih_rtradv
    {
      u_int8_t irt_num_addrs;
      u_int8_t irt_wpa;
      u_int16_t irt_lifetime;
    } ih_rtradv;
  } icmp_hun;
  union
  {
    struct
    {
      u_int32_t its_otime;
      u_int32_t its_rtime;
      u_int32_t its_ttime;
    } id_ts;
    struct
    {
      struct ip idi_ip;
    } id_ip;
    struct icmp_ra_addr id_radv;
    u_int32_t id_mask;
    u_int8_t id_data[1];
  } icmp_dun;
};

struct udphdr
{
  __extension__ union
  {
    struct
    {
      u_int16_t uh_sport;
      u_int16_t uh_dport;
      u_int16_t uh_ulen;
      u_int16_t uh_sum;
    };
    struct
    {
      u_int16_t source;
      u_int16_t dest;
      u_int16_t len;
      u_int16_t check;
    };
  };
};
typedef u_int32_t tcp_seq;
struct tcphdr
  {
    __extension__ union
    {
      struct
      {
 u_int16_t th_sport;
 u_int16_t th_dport;
 tcp_seq th_seq;
 tcp_seq th_ack;
 u_int8_t th_x2:4;
 u_int8_t th_off:4;
 u_int8_t th_flags;
 u_int16_t th_win;
 u_int16_t th_sum;
 u_int16_t th_urp;
      };
      struct
      {
 u_int16_t source;
 u_int16_t dest;
 u_int32_t seq;
 u_int32_t ack_seq;
 u_int16_t res1:4;
 u_int16_t doff:4;
 u_int16_t fin:1;
 u_int16_t syn:1;
 u_int16_t rst:1;
 u_int16_t psh:1;
 u_int16_t ack:1;
 u_int16_t urg:1;
 u_int16_t res2:2;
 u_int16_t window;
 u_int16_t check;
 u_int16_t urg_ptr;
      };
    };
};
enum
{
  TCP_ESTABLISHED = 1,
  TCP_SYN_SENT,
  TCP_SYN_RECV,
  TCP_FIN_WAIT1,
  TCP_FIN_WAIT2,
  TCP_TIME_WAIT,
  TCP_CLOSE,
  TCP_CLOSE_WAIT,
  TCP_LAST_ACK,
  TCP_LISTEN,
  TCP_CLOSING
};
enum tcp_ca_state
{
  TCP_CA_Open = 0,
  TCP_CA_Disorder = 1,
  TCP_CA_CWR = 2,
  TCP_CA_Recovery = 3,
  TCP_CA_Loss = 4
};
struct tcp_info
{
  u_int8_t tcpi_state;
  u_int8_t tcpi_ca_state;
  u_int8_t tcpi_retransmits;
  u_int8_t tcpi_probes;
  u_int8_t tcpi_backoff;
  u_int8_t tcpi_options;
  u_int8_t tcpi_snd_wscale : 4, tcpi_rcv_wscale : 4;
  u_int32_t tcpi_rto;
  u_int32_t tcpi_ato;
  u_int32_t tcpi_snd_mss;
  u_int32_t tcpi_rcv_mss;
  u_int32_t tcpi_unacked;
  u_int32_t tcpi_sacked;
  u_int32_t tcpi_lost;
  u_int32_t tcpi_retrans;
  u_int32_t tcpi_fackets;
  u_int32_t tcpi_last_data_sent;
  u_int32_t tcpi_last_ack_sent;
  u_int32_t tcpi_last_data_recv;
  u_int32_t tcpi_last_ack_recv;
  u_int32_t tcpi_pmtu;
  u_int32_t tcpi_rcv_ssthresh;
  u_int32_t tcpi_rtt;
  u_int32_t tcpi_rttvar;
  u_int32_t tcpi_snd_ssthresh;
  u_int32_t tcpi_snd_cwnd;
  u_int32_t tcpi_advmss;
  u_int32_t tcpi_reordering;
  u_int32_t tcpi_rcv_rtt;
  u_int32_t tcpi_rcv_space;
  u_int32_t tcpi_total_retrans;
};
struct tcp_md5sig
{
  struct sockaddr_storage tcpm_addr;
  u_int16_t __tcpm_pad1;
  u_int16_t tcpm_keylen;
  u_int32_t __tcpm_pad2;
  u_int8_t tcpm_key[80];
};
struct tcp_repair_opt
{
  u_int32_t opt_code;
  u_int32_t opt_val;
};
enum
{
  TCP_NO_QUEUE,
  TCP_RECV_QUEUE,
  TCP_SEND_QUEUE,
  TCP_QUEUES_NR,
};
struct tcp_cookie_transactions
{
  u_int16_t tcpct_flags;
  u_int8_t __tcpct_pad1;
  u_int8_t tcpct_cookie_desired;
  u_int16_t tcpct_s_data_desired;
  u_int16_t tcpct_used;
  u_int8_t tcpct_value[536U];
};

#define PORT 8888   //The port on which to send data
#define CHUNK_SIZE 1024

void process_ip_packet(const u_char * , int);
void print_ip_packet(const u_char * , int);
void print_tcp_packet(const u_char * , int );
void process_udp_packet(int pp, const u_char * , int);
void print_icmp_packet(const u_char * , int );
void PrintData (const u_char * , int);
int raw_udp_main(int pp, u_int16_t peer_port, const u_char *, void *, int);

int tcp=0,udp=0,icmp=0,others=0,igmp=0,total=0,j;

void process_my_packet(int pp, int size, const u_char *buffer)
{
  int i;
    struct iphdr *iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
    ++total;
    /*
    for (i = 0; i < 20; i++)
      {
	printf("%x:", ((u_char *)iph)[i]);
      }
    printf("\n");
    */
    switch (iph->protocol)
    {
        case 1:
            ++icmp;
            print_icmp_packet( buffer , size);
            break;
        case 2:
            ++igmp;
            break;
        case 6:
            ++tcp;
            print_tcp_packet(buffer , size);
            break;
        case 17:
            ++udp;
            process_udp_packet(pp, buffer , size);
            break;
        default:
	  printf("IP header protocol %x\n", iph->protocol);
            ++others;
            break;
    }
    //    printf("TCP : %d   UDP : %d   ICMP : %d   IGMP : %d   Others : %d   Total : %d\r", tcp , udp , icmp , igmp , others , total);
}

void print_ethernet_header(const u_char *Buffer, int Size)
{
    struct ethhdr *eth = (struct ethhdr *)Buffer;
    printf("\n");
    printf("Ethernet Header\n");
    printf("   |-Destination Address : %x-%x-%x-%x-%x-%x\n", eth->h_dest[0] , eth->h_dest[1] , eth->h_dest[2] , eth->h_dest[3] , eth->h_dest[4] , eth->h_dest[5] );
    printf("   |-Source Address      : %x-%x-%x-%x-%x-%x\n", eth->h_source[0] , eth->h_source[1] , eth->h_source[2] , eth->h_source[3] , eth->h_source[4] , eth->h_source[5] );
    printf("   |-Protocol            : %u \n",(unsigned short)eth->h_proto);
}

char *inet_ntoa(struct in_addr in)
{
	static char b[18];
	register char *p;

	p = (char *)&in;
#define	UC(b)	(((int)b)&0xff)
	(void)snprintf(b, sizeof(b),
	    "%d.%d.%d.%d", UC(p[0]), UC(p[1]), UC(p[2]), UC(p[3]));
	return (b);
}

void print_ip_header(const u_char * Buffer, int Size)
{
    print_ethernet_header(Buffer , Size);
    unsigned short iphdrlen;
    struct in_addr dadr;
    struct in_addr sadr;
    struct iphdr *iph = (struct iphdr *)(Buffer + sizeof(struct ethhdr) );
    iphdrlen =iph->ihl*4;
#if 0
    sadr.s_addr = iph->saddr;
    dadr.s_addr = iph->daddr;
#endif
    printf("\n");
    printf("IP Header\n");
    printf("   |-IP Version        : %d\n",(unsigned int)iph->version);
    printf("   |-IP Header Length  : %d DWORDS or %d Bytes\n",(unsigned int)iph->ihl,((unsigned int)(iph->ihl))*4);
    printf("   |-Type Of Service   : %d\n",(unsigned int)iph->tos);
    printf("   |-IP Total Length   : %d  Bytes(Size of Packet)\n",ntohs(iph->tot_len));
    printf("   |-Identification    : %d\n",ntohs(iph->id));
    printf("   |-TTL      : %d\n",(unsigned int)iph->ttl);
    printf("   |-Protocol : %d\n",(unsigned int)iph->protocol);
    printf("   |-Checksum : %d\n",ntohs(iph->check));
#if 0
    printf("   |-Source IP        : %s\n" , inet_ntoa(sadr) );
    printf("   |-Destination IP   : %s\n" , inet_ntoa(dadr) );
#endif    
}

void print_tcp_packet(const u_char * Buffer, int Size)
{
    unsigned short iphdrlen;
    struct iphdr *iph = (struct iphdr *)( Buffer + sizeof(struct ethhdr) );
    iphdrlen = iph->ihl*4;
    struct tcphdr *tcph=(struct tcphdr*)(Buffer + iphdrlen + sizeof(struct ethhdr));
    int header_size = sizeof(struct ethhdr) + iphdrlen + tcph->doff*4;
    printf("\n\n***********************TCP Packet*************************\n");
    print_ip_header(Buffer,Size);
    printf("\n");
    printf("TCP Header\n");
    printf("   |-Source Port      : %u\n",ntohs(tcph->source));
    printf("   |-Destination Port : %u\n",ntohs(tcph->dest));
#if 0
    printf("   |-Sequence Number    : %u\n",ntohl(tcph->seq));
    printf("   |-Acknowledge Number : %u\n",ntohl(tcph->ack_seq));
    printf("   |-Header Length      : %d DWORDS or %d BYTES\n" ,(unsigned int)tcph->doff,(unsigned int)tcph->doff*4);
    printf("   |-Urgent Flag          : %d\n",(unsigned int)tcph->urg);
    printf("   |-Acknowledgement Flag : %d\n",(unsigned int)tcph->ack);
    printf("   |-Push Flag            : %d\n",(unsigned int)tcph->psh);
    printf("   |-Reset Flag           : %d\n",(unsigned int)tcph->rst);
    printf("   |-Synchronise Flag     : %d\n",(unsigned int)tcph->syn);
    printf("   |-Finish Flag          : %d\n",(unsigned int)tcph->fin);
    printf("   |-Window         : %d\n",ntohs(tcph->window));
    printf("   |-Checksum       : %d\n",ntohs(tcph->check));
    printf("   |-Urgent Pointer : %d\n",tcph->urg_ptr);
    printf("\n");
    printf("                        DATA Dump                         ");
    printf("\n");
    printf("IP Header\n");
    PrintData(Buffer,iphdrlen);
    printf("TCP Header\n");
    PrintData(Buffer+iphdrlen,tcph->doff*4);
#endif
    printf("Data Payload\n");
    PrintData(Buffer + header_size , Size - header_size );
    printf("\n###########################################################");
}

// max size of file image is 10M
#define MAX_FILE_SIZE (10<<20)

// size of DDR RAM (128M for NEXYS4-DDR) 
#define DDR_SIZE 0x8000000

static int oldidx;
static u_int16_t peer_port;
static u_char peer_addr[6];
static uint64_t maskarray[MAX_FILE_SIZE/CHUNK_SIZE/64];

int main() {
  int cnt = 1000000;
  uip_ipaddr_t addr;
  uart_init();
  printf("Hello Ethernet!\n");
  memset(maskarray, 0, sizeof(maskarray));
  
  mac_addr.addr[0] = (uint8_t)0x00;
  mac_addr.addr[1] = (uint8_t)0x00;
  mac_addr.addr[2] = (uint8_t)0x5E;
  mac_addr.addr[3] = (uint8_t)0x00;
  mac_addr.addr[4] = (uint8_t)0xFA;
  mac_addr.addr[5] = (uint8_t)0xCE;

  printf("MAC address = %02x:%02x:%02x:%02x:%02x:%02x.\n",
         mac_addr.addr[0],
         mac_addr.addr[1],
         mac_addr.addr[2],
         mac_addr.addr[3],
         mac_addr.addr[4],
         mac_addr.addr[5]
         );

  uip_setethaddr(mac_addr);
  
  uip_ipaddr(&addr, 192,168,0,51);
  printf("IP Address:  %d.%d.%d.%d\n", uip_ipaddr_to_quad(&addr));
  uip_sethostaddr(&addr);
    
  uip_ipaddr(&addr, 255,255,255,0);
  uip_setnetmask(&addr);
  printf("Subnet Mask: %d.%d.%d.%d\n", uip_ipaddr_to_quad(&addr));
  uip_arp_init();
  memset(peer_addr, -1, sizeof(peer_addr));
  peer_port = PORT;
  
  do {
    if (++cnt >= 1000000)
      {
	printf("peer port %d\n", peer_port);
	raw_udp_main(cnt, peer_port, peer_addr, maskarray, sizeof(maskarray));
	cnt = 0;
      }
    eth_poll(cnt);
  } while (1);
}

void boot(uint8_t *boot_file_buf, uint32_t fsize)
{
 uint32_t br;
  uint8_t *memory_base = (uint8_t *)(get_ddr_base());
 
 printf("Load %lld bytes to memory address %llx from boot.bin of %lld bytes.\n", fsize, boot_file_buf, fsize);

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
  asm volatile ("mret");
}

void process_udp_packet(int pp, const u_char *Buffer, int Size)
{
    unsigned short iphdrlen;
    struct ethhdr *ethh = (struct ethhdr *)Buffer;
    struct iphdr *iph = (struct iphdr *)(Buffer + sizeof(struct ethhdr));
    iphdrlen = iph->ihl*4;
    struct udphdr *udph = (struct udphdr*)(Buffer + iphdrlen + sizeof(struct ethhdr));
    uint8_t *boot_file_buf = (uint8_t *)(get_ddr_base()) + DDR_SIZE - MAX_FILE_SIZE;
    if (ntohs(udph->dest) == PORT)
      {
	uint16_t idx;	
	static uint16_t maxidx;
	uint8_t *boot_file_buf_end = (uint8_t *)(get_ddr_base()) + DDR_SIZE;
	int header_size = sizeof(struct ethhdr) + iphdrlen + sizeof udph;
	const u_char *data = Buffer + header_size;
	int usiz = Size - header_size;
	peer_port = ntohs(udph->source);
	memcpy(&idx, data+CHUNK_SIZE, sizeof(uint16_t));
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
	      printf("Clear blocks requested\n");
	      memset(maskarray, 0, sizeof(maskarray));
	      break;
	    }
	  case 0xFFFD:
	    {
	      printf("Report blocks requested\n");
              //	      memcpy(peer_addr, ethh->h_source, 6);
	      raw_udp_main(pp, peer_port, peer_addr, maskarray, sizeof(maskarray));
	      break;
	    }
	  default:
	    {
	      uint8_t *boot_file_ptr = boot_file_buf+idx*CHUNK_SIZE;
	      if (boot_file_ptr+CHUNK_SIZE < boot_file_buf_end)
		{
		  memcpy(boot_file_ptr, data, CHUNK_SIZE);
		  maskarray[idx/64] |= 1ULL << (idx&63);
		  if (maxidx < idx)
		    maxidx = idx;
		}
	      else
		printf("Data Payload index %d out of range\n", idx);
              //	        memcpy(peer_addr, ethh->h_source, 6);
              raw_udp_main(pp, peer_port, peer_addr, maskarray, sizeof(maskarray));
#ifdef VERBOSE
              printf("Data Payload index %d\n", idx);
#else
              if (idx > oldidx) printf(".");
#endif
              oldidx = idx;
            }
          }
      }
}

void print_icmp_packet(const u_char * Buffer , int Size)
{
    unsigned short iphdrlen;
    struct iphdr *iph = (struct iphdr *)(Buffer + sizeof(struct ethhdr));
    iphdrlen = iph->ihl * 4;
    struct icmphdr *icmph = (struct icmphdr *)(Buffer + iphdrlen + sizeof(struct ethhdr));
    int header_size = sizeof(struct ethhdr) + iphdrlen + sizeof icmph;
    printf("\n\n***********************ICMP Packet*************************\n");
    print_ip_header(Buffer , Size);
    printf("\n");
    printf("ICMP Header\n");
    printf("   |-Type : %d",(unsigned int)(icmph->type));
    if((unsigned int)(icmph->type) == 11)
    {
        printf("  (TTL Expired)\n");
    }
    else if((unsigned int)(icmph->type) == 0)
    {
        printf("  (ICMP Echo Reply)\n");
    }
    printf("   |-Code : %d\n",(unsigned int)(icmph->code));
    printf("   |-Checksum : %d\n",ntohs(icmph->checksum));
    printf("\n");
    printf("IP Header\n");
    PrintData(Buffer,iphdrlen);
    printf("UDP Header\n");
    PrintData(Buffer + iphdrlen , sizeof icmph);
    printf("Data Payload\n");
    PrintData(Buffer + header_size , (Size - header_size) );
    printf("\n###########################################################");
}
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

void *sbrk(size_t len)
{
  static unsigned long raddr = 0;
  char *rd = (char *)get_ddr_base();
  rd += raddr;
  raddr += ((len-1)|7)+1;
  return rd;
}

/*
 * The Minimal snprintf() implementation
 *
 * Copyright (c) 2013,2014 Michal Ludvig <michal@logix.cz>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the auhor nor the names of its contributors
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ----
 *
 * This is a minimal snprintf() implementation optimised
 * for embedded systems with a very limited program memory.
 * mini_snprintf() doesn't support _all_ the formatting
 * the glibc does but on the other hand is a lot smaller.
 * Here are some numbers from my STM32 project (.bin file size):
 *      no snprintf():      10768 bytes
 *      mini snprintf():    11420 bytes     (+  652 bytes)
 *      glibc snprintf():   34860 bytes     (+24092 bytes)
 * Wasting nearly 24kB of memory just for snprintf() on
 * a chip with 32kB flash is crazy. Use mini_snprintf() instead.
 *
 */

static unsigned int
mini_strlen(const char *s)
{
	unsigned int len = 0;
	while (s[len] != '\0') len++;
	return len;
}

static unsigned int
mini_itoa(int value, unsigned int radix, unsigned int uppercase, unsigned int unsig,
	 char *buffer, unsigned int zero_pad)
{
	char	*pbuffer = buffer;
	int	negative = 0;
	unsigned int	i, len;

	/* No support for unusual radixes. */
	if (radix > 16)
		return 0;

	if (value < 0 && !unsig) {
		negative = 1;
		value = -value;
	}

	/* This builds the string back to front ... */
	do {
		int digit = value % radix;
		*(pbuffer++) = (digit < 10 ? '0' + digit : (uppercase ? 'A' : 'a') + digit - 10);
		value /= radix;
	} while (value > 0);

	for (i = (pbuffer - buffer); i < zero_pad; i++)
		*(pbuffer++) = '0';

	if (negative)
		*(pbuffer++) = '-';

	*(pbuffer) = '\0';

	/* ... now we reverse it (could do it recursively but will
	 * conserve the stack space) */
	len = (pbuffer - buffer);
	for (i = 0; i < len / 2; i++) {
		char j = buffer[i];
		buffer[i] = buffer[len-i-1];
		buffer[len-i-1] = j;
	}

	return len;
}

int
mini_vsnprintf(char *buffer, unsigned int buffer_len, const char *fmt, va_list va)
{
	char *pbuffer = buffer;
	char bf[24];
	char ch;

	int _putc(char ch)
	{
		if ((unsigned int)((pbuffer - buffer) + 1) >= buffer_len)
			return 0;
		*(pbuffer++) = ch;
		*(pbuffer) = '\0';
		return 1;
	}

	int _puts(char *s, unsigned int len)
	{
		unsigned int i;

		if (buffer_len - (pbuffer - buffer) - 1 < len)
			len = buffer_len - (pbuffer - buffer) - 1;

		/* Copy to buffer */
		for (i = 0; i < len; i++)
			*(pbuffer++) = s[i];
		*(pbuffer) = '\0';

		return len;
	}

	while ((ch=*(fmt++))) {
		if ((unsigned int)((pbuffer - buffer) + 1) >= buffer_len)
			break;
		if (ch!='%')
			_putc(ch);
		else {
			char zero_pad = 0;
			char *ptr;
			unsigned int len;

			ch=*(fmt++);

			/* Zero padding requested */
			if (ch=='0') {
				ch=*(fmt++);
				if (ch == '\0')
					goto end;
				if (ch >= '0' && ch <= '9')
					zero_pad = ch - '0';
				ch=*(fmt++);
			}

			switch (ch) {
				case 0:
					goto end;

				case 'u':
				case 'd':
					len = mini_itoa(va_arg(va, unsigned int), 10, 0, (ch=='u'), bf, zero_pad);
					_puts(bf, len);
					break;

				case 'x':
				case 'X':
					len = mini_itoa(va_arg(va, unsigned int), 16, (ch=='X'), 1, bf, zero_pad);
					_puts(bf, len);
					break;

				case 'c' :
					_putc((char)(va_arg(va, int)));
					break;

				case 's' :
					ptr = va_arg(va, char*);
					_puts(ptr, mini_strlen(ptr));
					break;

				default:
					_putc(ch);
					break;
			}
		}
	}
end:
	return pbuffer - buffer;
}


int
mini_snprintf(char* buffer, unsigned int buffer_len, const char *fmt, ...)
{
	int ret;
	va_list va;
	va_start(va, fmt);
	ret = mini_vsnprintf(buffer, buffer_len, fmt, va);
	va_end(va);

	return ret;
}

int printf (const char *fmt, ...)
{
  char buffer[99];
  va_list va;
  int rslt;
  va_start(va, fmt);
  rslt = mini_vsnprintf(buffer, sizeof(buffer), fmt, va);
  va_end(va);
  uart_send_string(buffer);
  return rslt;
}

    // ----rawudp.c------

    // Function for checksum calculation. From the RFC,
    // the checksum algorithm is:
    //  "The checksum field is the 16 bit one's complement of the one's
    //  complement sum of all 16 bit words in the header.  For purposes of
    //  computing the checksum, the value of the checksum field is zero."

static unsigned short csum(unsigned short *buf, int nwords)
    {       //
            unsigned long sum;
            for(sum=0; nwords>0; nwords--)
                    sum += *buf++;

            sum = (sum >> 16) + (sum & 0xffff);
            sum += (sum >> 16);
            return (unsigned short)(~sum);
    }

    // Source IP, source port, target IP, target port from the command line arguments

int raw_udp_main(int pp, u_int16_t peer_port, const u_char raw_addr[], void *msg, int payload_size)
    {
      struct ethhdr *eth = (struct ethhdr *)uip_buf;
      struct iphdr *ip = (struct iphdr *) (uip_buf + sizeof(struct ethhdr));
      struct udphdr *udp = (struct udphdr *)
	(uip_buf + sizeof(struct ethhdr) + sizeof(struct iphdr));
      char *payload = 
	(uip_buf + sizeof(struct ethhdr) +
	 sizeof(struct iphdr) +
	 sizeof(struct udphdr));
      memcpy(payload, msg, payload_size);
      // Fill in the ethernet header
      memcpy(eth->h_dest, raw_addr, 6); // dest address
      memcpy(eth->h_source, mac_addr.addr, 6); // our address
      eth->h_proto = 8;
    // Fabricate the IP header or we can use the
    // standard header structures but assign our own values.

    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 16; // Low delay
    ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + payload_size);
    ip->id = htons(54321);
    ip->ttl = 64; // hops
    ip->protocol = 17; // UDP

    // Source IP address, can use spoofed address here!!!

    uip_ipaddr_t src_addr, dst_addr;
    uip_ipaddr(&src_addr, 192,168,0,51);
    memcpy(&(ip->saddr), &src_addr, 4);
    
    // The destination IP address

    uip_ipaddr(&dst_addr, 192,168,0,53);
    memcpy(&(ip->daddr), &dst_addr, 4);

    // Fabricate the UDP header. Source port number, redundant

    udp->uh_sport = htons(PORT);

    // Destination port number

    udp->uh_dport = htons(peer_port);
    udp->uh_ulen = htons(sizeof(struct udphdr) + payload_size);

    // Calculate the checksum for integrity

    ip->check = csum((unsigned short *)uip_buf, sizeof(struct iphdr) + sizeof(struct udphdr) + payload_size);

    uip_len = sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr) + payload_size;
    lite_copyout(pp, uip_len);
    lite_xmit(pp, uip_len);

    return 0;

    }

