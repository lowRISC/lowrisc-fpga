# Xilinx Vivado script
# Version: Vivado 2015.4
# Function:
#   Download bitstream to FPGA

set boot [lindex $argv 2]
set bit [lindex $argv 1]
set device [lindex $argv 0]

puts "BOOT: $boot"
puts "BITSTREAM: $bit"
puts "DEVICE: $device"

write_cfgmem -force -format mcs -interface spix4 -size 128 -loadbit "up 0x0 $bit" -loaddata "up 0x00400000 $boot" -file "$bit.mcs"
