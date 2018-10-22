# See LICENSE.Cambridge for license details.
# build commands for Ethernet downloader
# Needs to be portable to Linux and FreeBSD

O = recvRawEth.o hash-md5.o
CFLAGS = -g
recvRawEth: $O
	$(CC) $O -o $@ $(CFLAGS)
