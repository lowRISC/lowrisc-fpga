#!/usr/bin/perl -w
# See LICENSE.Cambridge for license details.
# Endian conversion script (not sure if we still use it)

open (my $file, '<', $ARGV[0]) or die 'cannot read input mem file';

while(<$file>) {
    #print;
    chomp;
    @n = unpack("C*", $_);
    for(my $i=0; $i<@n/8; $i++) {
        print pack("C*", 
            $n[$i*8+6], $n[$i*8+7],
            $n[$i*8+4], $n[$i*8+5],
            $n[$i*8+2], $n[$i*8+3],
            $n[$i*8+0], $n[$i*8+1]);
    }
    print "\n"
}

