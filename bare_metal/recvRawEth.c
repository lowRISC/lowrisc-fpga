/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
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
 
#define DEST_MAC0	0xf4
#define DEST_MAC1	0xf2
#define DEST_MAC2	0x6d
#define DEST_MAC3	0x03
#define DEST_MAC4	0xb7
#define DEST_MAC5	0xc5

#define ETHER_TYPE	0x0800

#define DEFAULT_IF	"eth0"
#define BUF_SIZ		1536
// max size of file image is 10M
#define MAX_FILE_SIZE (10<<20)
#define CHUNK_SIZE 1024

static uint64_t maskarray[MAX_FILE_SIZE/CHUNK_SIZE/64];
struct sockaddr_in si_other;
char message[BUFLEN];

void send_message(int s, uint16_t idx)
{
  memcpy(message+CHUNK_SIZE, &idx, sizeof(uint16_t));
  //send the message
  if (sendto(s,
	     message,
	     CHUNK_SIZE+sizeof(uint16_t),
	     0,
	     (struct sockaddr *) &si_other,
	     sizeof(si_other)) == -1) die("sendto()");
}

int main(int argc, char *argv[])
{
  char sender[INET6_ADDRSTRLEN];
  int sockfd, ret, i;
  int sockopt;
  ssize_t numbytes;
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
  printf("File = %s, len = %d, chunks = %d\n", argv[1], len, chunks);
  assert(chunks < 65536);
  m = (char *)mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
  
  /* Header structures */
  struct ether_header *eh = (struct ether_header *) buf;
  struct iphdr *iph = (struct iphdr *) (buf + sizeof(struct ether_header));
  struct udphdr *udph = (struct udphdr *) (buf + sizeof(struct iphdr) + sizeof(struct ether_header));
  uint8_t *payload = (buf + sizeof(struct iphdr) + sizeof(struct ether_header) + sizeof(struct udphdr));
  
  memset(&if_ip, 0, sizeof(struct ifreq));
  
  /* Open PF_PACKET socket, listening for EtherType ETHER_TYPE */
  if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETHER_TYPE))) == -1) {
    perror("listener: socket");	
    return -1;
  }
  
  /* Set interface to promiscuous mode - do we need to do this every time? */
  strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
  ioctl(sockfd, SIOCGIFFLAGS, &ifopts);
#if 0
  ifopts.ifr_flags |= IFF_PROMISC;
  ioctl(sockfd, SIOCSIFFLAGS, &ifopts);
#endif	
  /* Allow the socket to be reused - incase connection is closed prematurely */
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
    perror("setsockopt");
    close(sockfd);
    exit(EXIT_FAILURE);
  }
  /* Bind to device */
  if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, ifName, IFNAMSIZ-1) == -1)	{
    perror("SO_BINDTODEVICE");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  /* set read timeout */
  struct timeval read_timeout;
  read_timeout.tv_sec = 0;
  read_timeout.tv_usec = 10000;
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

  send_message(s, 0xFFFC);
  send_message(s, 0xFFFE);
  
  while (incomplete)
    {
      incomplete = 0;
      for (idx = 0; idx < chunks; idx++)
	{
	  if (!(maskarray[idx/64] & (1ULL << (idx&63))))
	    {
	      printf("%d\n", idx);
	      ++incomplete;
	      memcpy(message, m+idx*CHUNK_SIZE, CHUNK_SIZE);
	      //send the message
	      send_message(s, idx);
	      do {
		numbytes = recvfrom(sockfd, buf, BUF_SIZ, 0, NULL, NULL);
		if (numbytes > 0)
		  {
		    /* Check the packet is for me */
		    if (eh->ether_dhost[0] == DEST_MAC0 &&
			eh->ether_dhost[1] == DEST_MAC1 &&
			eh->ether_dhost[2] == DEST_MAC2 &&
			eh->ether_dhost[3] == DEST_MAC3 &&
			eh->ether_dhost[4] == DEST_MAC4 &&
			eh->ether_dhost[5] == DEST_MAC5) {
		      /* UDP payload length */
		      ret = ntohs(udph->len) - sizeof(struct udphdr);
		      printf("listener: got packet %lu bytes\n", ret);
		      if (ret == sizeof(maskarray))
			memcpy(maskarray, payload, ret);
		    }
		  }
	      }
	      while (numbytes > 0);
	    }
	}
      send_message(s, 0xFFFD);
    }

  send_message(s, 0xFFFF);
  close(s);
  close(sockfd);
  return ret;
}
