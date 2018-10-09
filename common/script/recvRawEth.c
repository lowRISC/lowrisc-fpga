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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <fcntl.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <assert.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <openssl/md5.h>
#include <netdb.h>

#define SERVER "192.168.0.51"
#define BUFLEN 1536  //Max length of buffer
#define PORT "8888"   //The port on which to send data
#define CHUNK_SIZE 1464

enum {sizeof_maskarray=CHUNK_SIZE};

void die(char *s)
{
    perror(s);
    exit(1);
}

static uint64_t maskarray[sizeof_maskarray/8];
struct sockaddr_in si_other;
char message[BUFLEN], digest[MD5_DIGEST_LENGTH*2+1];

void send_message(int s, uint16_t idx)
{
  int len = CHUNK_SIZE+sizeof(uint16_t);
  memcpy(message+CHUNK_SIZE, &idx, sizeof(uint16_t));
  //send the message
  if (write(s, message, len) != len)
    die("send_message()");
}

int recv_message(int s, int typ)
{
  int update = 0;
  int len = 0;
  uint8_t payload[BUFLEN];
  do {
    len = read(s, payload, BUFLEN);
    if (len > 0)
      {
#if 1
          printf("listener: got packet %u bytes\n", len);
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
  while ((len > 0) && !update);
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
  int i, ret = 0;
  int sockopt, restart = 0;
  int md5digest = 0;
  int go = 1;
  int oldpercent = -1;
  struct ifreq ifopts;	/* set promiscuous mode */
  struct ifreq if_ip;	/* get ip addr */
  struct sockaddr_storage their_addr;
  uint8_t buf[BUFLEN];
  char ifName[IFNAMSIZ];
  socklen_t peer_addr_size;
  int cfd, len, chunks, fd, s, slen, rslt;
  int incomplete = 1;
  uint16_t idx;
  char *m;
  char *server = SERVER;
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  
  if (!strcmp(argv[1], "-r"))
    {
      restart = 1;
      ++argv;
      --argc;
    }

  if (!strcmp(argv[1], "-d"))
    {
      md5digest = 1;
      ++argv;
      --argc;
    }
  
  if (!strcmp(argv[1], "-s"))
    {
      server = argv[2];
      argv += 2;
      argc -= 2;
    }
  printf("Server(target) set to %s\n", server);

  if (argc < 2)
    die("args: [options] file (e.g. boot.bin)");

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
  hints.ai_flags = 0;
  hints.ai_protocol = 0;          /* Any protocol */
  
  s = getaddrinfo(server, PORT, &hints, &result);
  if (s != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
  }
  
  /* getaddrinfo() returns a list of address structures.
     Try each address until we successfully connect(2).
     If socket(2) (or connect(2)) fails, we (close the socket
     and) try the next address. */
  
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    s = socket(rp->ai_family, rp->ai_socktype,
                 rp->ai_protocol);
    if (s == -1)
      continue;
    
    if (connect(s, rp->ai_addr, rp->ai_addrlen) != -1)
      break;                  /* Success */
    
    close(s);
  }
  
  if (rp == NULL) {               /* No address succeeded */
    fprintf(stderr, "Could not connect\n");
    exit(EXIT_FAILURE);
  }
  
  freeaddrinfo(result);           /* No longer needed */

  if (ifName[0] == '-' || argc > 3)
    {
      fprintf(stderr, "Unhandled option %s\n", argv[1]);
      exit(1);
    }
    
  fd = open(argv[1], O_RDONLY);
  if (fd < 0)
    {
      perror(argv[1]);
    }
  len = lseek(fd, 0, SEEK_END);
  chunks = (len+CHUNK_SIZE-1) / CHUNK_SIZE;
  printf("File = %s, len = %d, chunks = %d\n", argv[1], len, chunks);
  assert(chunks < 65536);
  m = (char *)mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
  unsigned char md[MD5_DIGEST_LENGTH];
  unsigned char hex[MD5_DIGEST_LENGTH*2+1];
  /* Header structures */
  struct ether_header *eh = (struct ether_header *) buf;
  struct ip *iph = (struct ip *) (buf + sizeof(struct ether_header));
  struct udphdr *udph = (struct udphdr *) (buf + sizeof(struct ip) + sizeof(struct ether_header));
  
  memset(&if_ip, 0, sizeof(struct ifreq));
  
  /* restart */
  while (restart)
    {
      printf("Restarting\n");
      do {
        send_message(s, 0xFFFE);
        usleep(1000);
      }
      while (!recv_message(s, sizeof(maskarray)));
      restart = 0;
      for (i = 0; i < sizeof(maskarray)/sizeof(*maskarray); i++)
        {
          if (maskarray[i]) restart = 1;
        }
    }
  
  while (incomplete)
    {
      enum {wait=2, dly=400};
      int cnt = 0;
      for (idx = 0; idx < chunks; ++idx)
	{
	  if (!(maskarray[idx/64] & (1ULL << (idx&63))))
	    {
	      memcpy(message, m+idx*CHUNK_SIZE, CHUNK_SIZE);
	      //send the message
	      send_message(s, idx);
              if (++cnt % wait == 0) usleep(dly);
	    }
	}
      do {
        send_message(s, 0xFFFD);
        usleep(1000);
      }
      while (!recv_message(s, sizeof(maskarray)));
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
  if (go) send_message(s, 0xFFFF);
  close(s);
  return ret;
}
