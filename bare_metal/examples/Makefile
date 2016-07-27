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
GEN_DIR = $(FPGA_DIR)/generated-src
RISCV_PREFIX=riscv64-unknown-elf-
RISCV_GCC = $(RISCV_PREFIX)gcc
RISCV_DUMP = $(RISCV_PREFIX)objdump
RISCV_GCC_OPTS = -mcmodel=medany -fpie -static -std=gnu99 -g -ffast-math -fno-common -fno-builtin-printf -I$(DRIVER_DIR) -I$(GEN_DIR)
RISCV_DUMP_OPTS = -D -S -l
RISCV_LINK = $(RISCV_GCC) -T $(DRIVER_DIR)/test.ld
RISCV_LINK_OPTS = -nostdlib -nostartfiles -ffast-math -lc -lgcc

#--------------------------------------------------------------------
# Objects
#--------------------------------------------------------------------

default: all

TARGETS = boot dram hello sdcard trace jump flash

DRIVER_TARGETS = syscalls spi uart memory elf diskio ff

HEADERS += $(wildcard $(DRIVER_DIR)/*.h) $(GEN_DIR)/dev_map.h

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
	$(RISCV_LINK) $(RISCV_GCC_OPTS) -o $@ $< $(DRIVER_OBJS) crt.o $(RISCV_LINK_OPTS)

$(HEX_FILES): %.hex:%.riscv
	elf2hex 1073741824 16 4096 $< > $@

$(DUMP_FILES): %.dump:%.riscv
	$(RISCV_DUMP) $(RISCV_DUMP_OPTS) $< > $@

.PHONY: all dump
junk += $(RISCV_FILES) $(HEX_FILES) $(DUMP_FILES)

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
