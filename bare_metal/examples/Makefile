#=======================================================================
# Makefile for all KC705 examples
#-----------------------------------------------------------------------
# See LICENSE for license details.

# check RISCV environment variable
ifndef RISCV
$(error Please set environment variable RISCV. Please take a look at README)
endif

#--------------------------------------------------------------------
# Build rules
#--------------------------------------------------------------------

FPGA_DIR ?= $(TOP)/vsim
BASE_DIR ?= .

DRIVER_DIR = $(BASE_DIR)/../driver
RISCV_PREFIX=riscv64-unknown-elf-
RISCV_GCC = $(RISCV_PREFIX)gcc
RISCV_DUMP = $(RISCV_PREFIX)objdump
RISCV_GCC_OPTS = -static -std=gnu99 -g -Os -ffast-math -fno-common -fno-builtin-printf -mcmodel=medany -I$(DRIVER_DIR) -I$(BASE_DIR)
RISCV_DUMP_OPTS = -D -S -l
RISCV_LINK = $(RISCV_PREFIX)gcc -T $(DRIVER_DIR)/test.ld
RISCV_LINK_OPTS = -nostdlib -nostartfiles -ffast-math -Xlinker -M=mapfile
# -march=rv64imafd

#--------------------------------------------------------------------
# Objects
#--------------------------------------------------------------------

default: all

TARGETS = dram hello sdcard trace jump flash tag selftest eth

DRIVER_TARGETS = syscalls hid memory memcmp memset strcmp strcpy strnlen wordcopy elf sdhci-minion-hash-md5 mini-printf random dhcp-client ethlib stublib # minion_helper diskio ff 

HEADERS += $(wildcard $(DRIVER_DIR)/*.h)

#--------------------------------------------------------------------
# Build Drivers
#--------------------------------------------------------------------

DRIVER_OBJS = $(addsuffix .o, $(DRIVER_TARGETS))

$(DRIVER_OBJS): %.o:$(DRIVER_DIR)/%.c $(HEADERS)
	$(RISCV_GCC) $(RISCV_GCC_OPTS) -c $< -o $@

crt.o: $(DRIVER_DIR)/crt.S
	$(RISCV_GCC) $(RISCV_GCC_OPTS) -c $< -o $@

junk += $(DRIVER_OBJS) crt.o

#--------------------------------------------------------------------
# Building Targets
#--------------------------------------------------------------------

RISCV_FILES = $(addsuffix .riscv, $(TARGETS))
HEX_FILES = $(addsuffix .hex, $(TARGETS))
DUMP_FILES = $(addsuffix .dump, $(TARGETS))

all: $(HEX_FILES)

dump: $(DUMP_FILES)

$(RISCV_FILES): %.riscv:$(BASE_DIR)/%.c $(HEADERS) $(DRIVER_OBJS) crt.o $(DRIVER_DIR)/test.ld
	$(RISCV_LINK) $(RISCV_GCC_OPTS) $(RISCV_LINK_OPTS) -o $@ $< $(DRIVER_OBJS) crt.o
	riscv64-unknown-elf-objdump -d $@ > $*.dis

$(HEX_FILES): %.hex:%.riscv
	riscv64-unknown-elf-objcopy -I elf64-little -O verilog $< cnvmem.mem
	iverilog ../script/cnvmem.v -o cnvmem
	./cnvmem
	mv cnvmem.coe $*.coe
	mv cnvmem.hex $@

$(DUMP_FILES): %.dump:%.riscv
	$(RISCV_DUMP) $(RISCV_DUMP_OPTS) $< > $@

.PHONY: all dump
junk += $(RISCV_FILES) $(HEX_FILES) $(DUMP_FILES)

U = lib/strto.c  lib/time.c  lib/div64.c  lib/ctype.c   drivers/block/blk_legacy.c  drivers/mmc/lowrisc_mmc.c  drivers/mmc/mmc_legacy.c \
drivers/mmc/mmc.c lib/string.c  lib/display_options.c ../driver/mini-printf.c ../driver/hid.c ../driver/syscalls.c boot.c ../driver/ff.c \
../driver/memory.c ../driver/sdhci-minion-hash-md5.c ../driver/elf.c ../driver/ethlib.c ../driver/dhcp-client.c ../driver/random.c \
# ../driver/minion_helper.c

MMC = blk_legacy.o crt.o ctype.o display_options.o div64.o elf.o ff.o hid.o lowrisc_mmc.o memory.o mini-printf.o mmc_legacy.o mmc.o sdhci-minion-hash-md5.o string.o strto.o syscalls.o time.o ethlib.o dhcp-client.o random.o

boot: $U
	riscv64-unknown-elf-gcc -c -o crt.o ../driver/crt.S
	riscv64-unknown-elf-gcc -c -g -Iinclude -I./arch/riscv/include -I../driver -include ./include/linux/kconfig.h -D__KERNEL__ -D__UBOOT__ -Wall -Wstrict-prototypes -Wno-format-security -fno-builtin -ffreestanding -fshort-wchar -Os -fno-stack-protector -fno-delete-null-pointer-checks -g -fstack-usage -Wno-format-nonliteral -ffixed-gp -fpic -fno-strict-aliasing -fno-common -gdwarf-2 -I. -fno-builtin-printf -fno-builtin-memcpy $U 
 # minion_helper.o
	riscv64-unknown-elf-gcc -o $@ -g -nostdlib -nostartfiles boot.o $(MMC) -T ../driver/test.ld

#--------------------------------------------------------------------
# clean up
#--------------------------------------------------------------------

clean:
	rm -rf $(junk)

.PHONY: clean


# emacs local variable

# Local Variables:
# mode: makefile
# End:
