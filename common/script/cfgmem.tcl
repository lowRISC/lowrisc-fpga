# Xilinx Vivado script
# Version: Vivado 2015.4
# Function:
#   Download bitstream to FPGA

set bit [lindex $argv 1]
set device [lindex $argv 0]

puts "BITSTREAM: $bit"
puts "DEVICE: $device"

write_cfgmem -force -format MCS -size 128 -interface BPIx16 -loadbit "up 0x0 $bit" "$bit.mcs"
