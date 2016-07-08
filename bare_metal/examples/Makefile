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

DRIVER_DIR = ../driver
GEN_DIR = $(FPGA_DIR)/generated-src
RISCV_PREFIX=riscv64-unknown-elf-
RISCV_GCC = $(RISCV_PREFIX)gcc
RISCV_DUMP = $(RISCV_PREFIX)objdump
RISCV_GCC_OPTS = -fpie -static -std=gnu99 -g -ffast-math -fno-common -fno-builtin-printf -I$(DRIVER_DIR) -I$(GEN_DIR)
RISCV_DUMP_OPTS = -D -S -l
RISCV_LINK = $(RISCV_GCC) -T $(DRIVER_DIR)/test.ld
RISCV_LINK_OPTS = -nostdlib -nostartfiles -ffast-math -lc -lgcc

#--------------------------------------------------------------------
# Objects
#--------------------------------------------------------------------

default: all

TARGETS = boot dram hello sdcard trace jump

DRIVER_OBJS += \
	$(DRIVER_DIR)/syscalls.o \
	$(DRIVER_DIR)/spi.o \
	$(DRIVER_DIR)/uart.o \
	$(DRIVER_DIR)/memory.o \
	$(DRIVER_DIR)/elf.o \
	$(DRIVER_DIR)/diskio.o \
	$(DRIVER_DIR)/ff.o \

HEADERS += \
	$(wildcard $(DRIVER_DIR)/*.h) \

junk += \
	$(addsuffix .riscv, $(TARGETS)) \
	$(addsuffix .hex, $(TARGETS)) \
	$(addsuffix .dump, $(TARGETS)) \
	$(DRIVER_OBJS) \

#--------------------------------------------------------------------
# Build Drivers
#--------------------------------------------------------------------

$(DRIVER_OBJS): %.o:%.c $(HEADERS)
	$(RISCV_GCC) $(RISCV_GCC_OPTS) -c $< -o $@

crt.o: $(DRIVER_DIR)/crt.S
	$(RISCV_GCC) $(RISCV_GCC_OPTS) -c $< -o $@

junk += crt.o

#--------------------------------------------------------------------
# Building Targets
#--------------------------------------------------------------------

all: $(addsuffix .hex, $(TARGETS))

dump: $(addsuffix .dump, $(TARGETS))

$(addsuffix .riscv, $(TARGETS)): %.riscv:%.c $(HEADERS) $(DRIVER_OBJS) crt.o $(DRIVER_DIR)/test.ld
	$(RISCV_LINK) $(RISCV_GCC_OPTS) -o $@ $< $(DRIVER_OBJS) crt.o $(RISCV_LINK_OPTS)

$(addsuffix .hex, $(TARGETS)): %.hex:%.riscv
	elf2hex 1073741824 16 4096 $< > $@

$(addsuffix .dump, $(TARGETS)): %.dump:%.riscv
	$(RISCV_DUMP) $(RISCV_DUMP_OPTS) $< > $@

.PHONY: all dump

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
