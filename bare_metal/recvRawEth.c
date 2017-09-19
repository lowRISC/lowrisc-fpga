/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 It may be useful to throttle the ethernet in conjunction with this program to
  take into account the latency of the target processor, for example:

  sudo tc qdisc add dev eth0 root tbf rate 4.0mbit latency 50ms burst 50kb mtu 10000

 */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <openssl/md5.h>

int select_wait(int sockfd)
{
  fd_set rfds;
  struct timeval tv;
  int retval;
  
  /* Watch sockfd to see when it has input. */
  FD_ZERO(&rfds);
  FD_SET(sockfd, &rfds);
  
  /* Wait up to five seconds. */
  tv.tv_sec = 0;
  tv.tv_usec = 10000;
  
  retval = select(1, &rfds, NULL, NULL, &tv);
  /* Don't rely on the value of tv now! */
  
  if (retval == -1)
    perror("select()");
  else if (retval)
    return 1;
  else
    return 0;
}

#define SERVER "192.168.0.51"
#define BUFLEN 1536  //Max length of buffer
#define PORT 8888   //The port on which to send data
#define CHUNK_SIZE 1024

void die(char *s)
{
    perror(s);
    exit(1);
}

#define ETHER_TYPE	0x0800

#define DEFAULT_IF	"eth0"
#define BUF_SIZ		1536
// max size of file image is 10M
#define MAX_FILE_SIZE (10<<20)
#define CHUNK_SIZE 1024

static uint64_t maskarray[MAX_FILE_SIZE/CHUNK_SIZE/64];
struct sockaddr_in si_other;
char message[BUFLEN], digest[MD5_DIGEST_LENGTH*2+1];

void send_message(int s, uint16_t idx)
{
  memcpy(message+CHUNK_SIZE, &idx, sizeof(uint16_t));
  //send the message
  if (sendto(s,
	     message,
	     CHUNK_SIZE+sizeof(uint16_t),
	     0,
	     (struct sockaddr *) &si_other,
	     sizeof(si_other)) == -1)
    die("sendto()");
  usleep(10000);
}

int recv_message(int sockfd, int typ)
{
  int update = 0;
  int numbytes;
  uint8_t buf[BUF_SIZ];
  /* Header structures */
  struct ether_header *eh = (struct ether_header *) buf;
  struct iphdr *iph = (struct iphdr *) (buf + sizeof(struct ether_header));
  struct udphdr *udph = (struct udphdr *) (buf + sizeof(struct iphdr) + sizeof(struct ether_header));
  uint8_t *payload = (buf + sizeof(struct iphdr) + sizeof(struct ether_header) + sizeof(struct udphdr));
  do {
    numbytes = recvfrom(sockfd, buf, BUF_SIZ, 0, NULL, NULL);
    if ((numbytes > sizeof(struct iphdr) + sizeof(struct ether_header) + sizeof(struct udphdr)) &&
        (ntohs(eh->ether_type)==ETH_P_IP) &&
        (iph->protocol==IPPROTO_UDP))
      {
        int len = ntohs(udph->len) - sizeof(struct udphdr);
        int sport = ntohs(udph->source);
        int dport = ntohs(udph->dest);
        /* Check the packet is for me */
          if ((sport == 8888) && (dport == 8888)) {
          /* UDP payload length */
#if 1
          printf("listener: got packet %lu bytes\n", len);
#endif          
          switch (len)
            {
            case sizeof(maskarray):
              memcpy(maskarray, payload, len);
              update = (len==typ);
              break;
            case MD5_DIGEST_LENGTH*2+1:
              memcpy(digest, payload, len);
              update = (len==typ);
              break;
            default:
              printf("Don't know what to do with message of length %d\n", len);
              break;
            }
        }
      }
  }
  while ((numbytes > 0) && !update);
  return update;
}

static void md5_bin2hex(char *p, const char *cp)
{
  static const char *hex = "0123456789abcdef";
  int count = 16;
  while (count) {
  unsigned char c = *cp++;

  *p++ = hex[c >> 4];
  *p++ = hex[c & 0xf];
  count--;
 }
  *p = 0;
}

int main(int argc, char *argv[])
{
  char sender[INET6_ADDRSTRLEN];
  int sockfd, i, ret = 0;
  int sockopt, restart = 0;
  int oldpercent = -1;
  struct ifreq ifopts;	/* set promiscuous mode */
  struct ifreq if_ip;	/* get ip addr */
  struct sockaddr_storage their_addr;
  uint8_t buf[BUF_SIZ];
  char ifName[IFNAMSIZ];
  socklen_t peer_addr_size;
  int cfd, len, chunks, fd, s, slen, rslt;
  int incomplete = 1;
  uint16_t idx;
  char *m;

  if (!strcmp(argv[1], "-r"))
    {
      restart = 1;
      ++argv;
      --argc;
    }
  
  /* Get interface name */
  if (argc < 3)
    die("args: interface (e.g. eth0) file (e.g. boot.bin)");

  memset((char *) &si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons(PORT);
     
  if (inet_aton(SERVER, &si_other.sin_addr) == 0) 
    {
      fprintf(stderr, "inet_aton() failed\n");
      exit(1);
    }    
    
  if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) die("socket");

  strcpy(ifName, argv[1]);
  fd = open(argv[2], O_RDONLY);
  len = lseek(fd, 0, SEEK_END);
  chunks = (len+CHUNK_SIZE-1) / CHUNK_SIZE;
  printf("File = %s, len = %d, chunks = %d\n", argv[2], len, chunks);
  assert(chunks < 65536);
  m = (char *)mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
  unsigned char md[MD5_DIGEST_LENGTH];
  unsigned char hex[MD5_DIGEST_LENGTH*2+1];
  uint8_t *md5 = MD5((unsigned char *)m, chunks*CHUNK_SIZE, md);
  md5_bin2hex(hex, md5);
  printf("MD5 sum = %s\n", hex);
  /* Header structures */
  struct ether_header *eh = (struct ether_header *) buf;
  struct iphdr *iph = (struct iphdr *) (buf + sizeof(struct ether_header));
  struct udphdr *udph = (struct udphdr *) (buf + sizeof(struct iphdr) + sizeof(struct ether_header));
  
  memset(&if_ip, 0, sizeof(struct ifreq));
  
  /* Open PF_PACKET socket, listening for EtherType ETHER_TYPE */
  if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETHER_TYPE))) == -1) {
    perror("listener: socket");	
    return -1;
  }
  
  /* Set interface to promiscuous mode - do we need to do this every time? */
  strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
  /* Bind to device */
  if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, ifName, IFNAMSIZ-1) == -1)	{
    perror("SO_BINDTODEVICE");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  /* set read timeout */
  struct timeval read_timeout;
  read_timeout.tv_sec = 0;
  read_timeout.tv_usec = 100;
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

  /* restart */
  while (restart)
    {
      printf("Restarting\n");
      do {
        send_message(s, 0xFFFE);
        usleep(100000);
      }
      while (!recv_message(sockfd, sizeof(maskarray)));
      restart = 0;
      for (i = 0; i < sizeof(maskarray)/sizeof(*maskarray); i++)
        {
          if (maskarray[i]) restart = 1;
        }
    }
  
  while (incomplete)
    {
      for (idx = 0; idx < chunks; ++idx)
	{
	  if (!(maskarray[idx/64] & (1ULL << (idx&63))))
	    {
	      memcpy(message, m+idx*CHUNK_SIZE, CHUNK_SIZE);
	      //send the message
	      send_message(s, idx);
	    }
	}
      do {
        send_message(s, 0xFFFD);
        usleep(1000000);
      }
      while (!recv_message(sockfd, sizeof(maskarray)));
      incomplete = 0;
      for (idx = 0; idx < chunks; ++idx)
	{
	  if (!(maskarray[idx/64] & (1ULL << (idx&63))))
	    {
	      ++incomplete;
	    }
	}
      printf(" %d%%\n", 100*(chunks-incomplete)/chunks);
      fflush(stdout);
    }
  do {
    send_message(s, 0xFFFC);
    usleep(1000000);
  }
  while (!recv_message(sockfd, MD5_DIGEST_LENGTH*2+1));
  printf("Received digest = %s", digest);
  if (!strcmp(digest, hex))
    {
    printf(" (OK)\n");
    send_message(s, 0xFFFF);
    }
  else
    printf(" (BAD)\n");
  close(s);
  close(sockfd);
  return ret;
}
