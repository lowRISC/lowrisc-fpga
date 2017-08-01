/*
    Simple udp client
*/
#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <sys/socket.h>
 
#define SERVER "192.168.0.51"
#define BUFLEN 1536  //Max length of buffer
#define PORT 8888   //The port on which to send data
#define CHUNK_SIZE 1024

void die(char *s)
{
    perror(s);
    exit(1);
}
 
int main(int argc, char **argv)
{
  struct sockaddr_in si_other;
  socklen_t peer_addr_size;
  int cfd, len, chunks, fd, s, slen, rslt;
  int incomplete = 1;
  char message[BUFLEN];
  uint16_t idx;
  char *m;

  memset((char *) &si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons(PORT);
     
  if (inet_aton(SERVER, &si_other.sin_addr) == 0) 
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }    
    
  if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) die("socket");

  fd = open(argv[1], O_RDONLY);
  len = lseek(fd, 0, SEEK_END);
  chunks = (len+CHUNK_SIZE-1) / CHUNK_SIZE;
  printf("File = %s, len = %d, chunks = %d\n", argv[1], len, chunks);
  assert(chunks < 65536);
  m = (char *)mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);

  while (incomplete)
    {
      for (idx = 0; idx < chunks; idx++)
	{
	  memcpy(message, m+idx*CHUNK_SIZE, CHUNK_SIZE);
	  memcpy(message+CHUNK_SIZE, &idx, sizeof(uint16_t));
	  //send the message
	  if (sendto(s,
		     message,
		     CHUNK_SIZE+sizeof(uint16_t),
		     0,
		     (struct sockaddr *) &si_other,
		     sizeof(si_other)) == -1) die("sendto()");
	  
	  usleep(1000);
	}
      //try to receive some data
      slen=sizeof(si_other);
      rslt = recvfrom(s,
		      message,
		      BUFLEN,
		      0 && MSG_DONTWAIT,
		      (struct sockaddr *) &si_other,
		      &slen);
      if (rslt == -1)
	die("recvfrom()");
    }
  
  close(s);
  return 0;
}
