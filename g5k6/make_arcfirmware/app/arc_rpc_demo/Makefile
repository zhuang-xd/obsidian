export SOC_ARCH = MC632X
export CHIP_ID = MC632X
CONFIG_PATH = .config

CHIP_LITTLE_CASE = $(shell echo $(SOC_ARCH) | tr A-Z a-z)

TOPDIR  =  $(shell pwd)/../..
LIB_DIR := $(TOPDIR)/out/libs

-include $(CONFIG_PATH)
TARGET = rtthread_arc.bin
ELF_FILE = $(TARGET:.bin=.elf)
MAP_FILE = $(TARGET:.bin=.map)
DIS_FILE = $(TARGET:.bin=.dis)

TEXT_BASE=0x7FC00000

GIT_VERSION = $(shell  git rev-parse --verify --short=8 HEAD)

export CROSS_COMPILE = arc-fullhan-elf32-
CFLAGS =  -include fhconfig.h
CFLAGS += -mno-sdata -mmul32x16 -mnorm -mA6
CFLAGS += -O3 -Wall -DCONFIG_ARCH_$(SOC_ARCH)
CFLAGS += -DCONFIG_CHIP_ARC -DCONFIG_CHIP_$(CHIP_ID) -D$(CHIP_ID)
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -mlong-calls
CFLAGS += -DFH_USING_RPC_SOLUTION -DFH_USING_DDRBOOT
# CFLAGS += -Werror
ARFLAGS=rcs
ASFLAGS=-mno-sdata -mmul32x16 -mnorm -mA6 -x assembler-with-cpp
ASFLAGS+=-I$(TOPDIR)/kernel/libcpu/arc/arc600

ASMSRC = $(TOPDIR)/kernel/libcpu/arc/arc600/startup_gcc.S
ASMSRC += $(TOPDIR)/kernel/libcpu/arc/arc600/irq_gcc.S
ASMSRC += $(TOPDIR)/kernel/libcpu/arc/arc600/context_gcc.S

ASMOBJS=$(patsubst %.S, %.o, $(ASMSRC))

# define your private sensor name
ifeq ($(FH_USING_PS5270_MIPI), y)
SENSOR = ps5270_mipi
endif

ifeq ($(FH_USING_OVOS02K_MIPI), y)
SENSOR = ovos02k_mipi
endif

ifeq ($(FH_USING_OVOS04C10_MIPI), y)
SENSOR = ovos04c10_mipi
endif

ifeq ($(FH_USING_OV04C10_MIPI), y)
SENSOR = ov04c10_mipi
endif

ifeq ($(FH_USING_OVOS08_MIPI), y)
SENSOR = ovos08_mipi
endif

ifeq ($(FH_USING_SC3336_MIPI), y)
SENSOR = sc3336_mipi
endif

ifeq ($(FH_USING_GC2083_MIPI), y)
SENSOR = gc2083_mipi
endif

# define your private sensor parameter source code here.
SENSOR_PARAM_DIR=$(shell pwd)/media/isp/src/sensor_param
SENSOR_PARAM_CSRC:=$(foreach sensor, $(SENSOR), $(SENSOR_PARAM_DIR)/sensorParam_$(sensor)_attr.c)

# define sensor macro
SENSOR_UPPER = $(shell echo $(SENSOR) | tr a-z A-Z)

# SUBDIRS目录下文件都会编译成.a
SUBDIRS := $(TOPDIR)/kernel
SUBDIRS += $(TOPDIR)/platform
SUBDIRS += $(TOPDIR)/components/vmm
SUBDIRS += $(TOPDIR)/components/runlog
SUBDIRS += $(TOPDIR)/components/ringbuffer
SUBDIRS += $(TOPDIR)/components/shell
SUBDIRS += $(TOPDIR)/components/timestat
SUBDIRS += $(TOPDIR)/components/profiling
SUBDIRS += $(TOPDIR)/components/newlib
SUBDIRS += $(TOPDIR)/components/xbus
SUBDIRS += $(TOPDIR)/drivers/dma
SUBDIRS += $(TOPDIR)/drivers/i2c
SUBDIRS += $(TOPDIR)/drivers/wdt
SUBDIRS += $(TOPDIR)/drivers/sadc
SUBDIRS += $(TOPDIR)/drivers/interrupt
SUBDIRS += $(TOPDIR)/drivers/gpio
SUBDIRS += $(TOPDIR)/drivers/pwm
SUBDIRS += $(TOPDIR)/drivers/uart
SUBDIRS += $(TOPDIR)/drivers/nn

INC_DIR = -I. -I$(TOPDIR)/lib/platform/$(CHIP_LITTLE_CASE)/
INC_DIR += -I$(TOPDIR)/lib/inc/rt-thread
INC_DIR += -I$(TOPDIR)/components/newlib
INC_DIR += -I$(TOPDIR)/components/timestat
INC_DIR += -I$(TOPDIR)/components/xbus/include
INC_DIR += -I$(TOPDIR)/lib/$(CHIP_LITTLE_CASE)/inc/
INC_DIR += -I$(TOPDIR)/lib/$(CHIP_LITTLE_CASE)/inc/dsp/types
INC_DIR += -I$(TOPDIR)/lib/$(CHIP_LITTLE_CASE)/inc/dsp
INC_DIR += -I$(TOPDIR)/lib/$(CHIP_LITTLE_CASE)/inc/isp
INC_DIR += -I$(TOPDIR)/lib/$(CHIP_LITTLE_CASE)/inc/vicap
INC_DIR += -I$(TOPDIR)/lib/$(CHIP_LITTLE_CASE)/inc/advapi
INC_DIR += -I$(TOPDIR)/lib/inc
INC_DIR += -I$(TOPDIR)/lib/inc/types
INC_DIR += -I$(TOPDIR)/customize

INC_DIR += -I$(TOPDIR)/components/xbus
INC_DIR += -I$(TOPDIR)/components/xbus/include
INC_DIR += -I$(TOPDIR)/platform
INC_DIR += -I$(TOPDIR)/platform/inc
INC_DIR += -I$(TOPDIR)/drivers/inc
INC_DIR += -I$(TOPDIR)/drivers/pwm


INC_DIR += -I$(TOPDIR)/app/arc_rpc_demo
INC_DIR += -I$(TOPDIR)/app/arc_rpc_demo/fast_nn
INC_DIR += -I$(TOPDIR)/app/arc_rpc_demo/rpc_customer
INC_DIR += -I$(TOPDIR)/app/arc_rpc_demo/media
INC_DIR += -I$(TOPDIR)/app/arc_rpc_demo/media/audio/inc
INC_DIR += -I$(TOPDIR)/app/arc_rpc_demo/media/dsp/inc
INC_DIR += -I$(TOPDIR)/app/arc_rpc_demo/media/isp/inc
INC_DIR += -I$(TOPDIR)/app/arc_rpc_demo/media/intelligent_det/md
INC_DIR += -I$(TOPDIR)/app/arc_rpc_demo/media/intelligent_det/nna
INC_DIR += -I$(TOPDIR)/app/arc_rpc_demo/media/overlay
INC_DIR += -I$(TOPDIR)/app/arc_rpc_demo/media/audio

LD_DIR = -L$(TOPDIR)/lib -L$(shell pwd)/
LD_DIR += -L$(TOPDIR)/lib/$(CHIP_LITTLE_CASE)
LD_DIR += -L$(TOPDIR)/lib/$(CHIP_LITTLE_CASE)/sensors
LD_DIR += -L$(LIB_DIR)

# define sensor libs
SENSOR_DIR = $(TOPDIR)/lib/$(CHIP_LITTLE_CASE)/sensors
SENSOR_LIB_FILES = $(notdir $(shell find $(SENSOR_DIR) -name *$(SENSOR)*.a))
SENSORS = $(basename $(SENSOR_LIB_FILES))
SENSOR_DATA  := $(SENSORS:lib%rtt=%sensor_if)
SENSOR_LIB = $(SENSORS:lib%=-l%)

CFLAGS += $(INC_DIR)

LIBFILE = $(notdir $(shell find $(TOPDIR)/lib/$(CHIP_LITTLE_CASE) -name *.a))

LIBNAME = $(patsubst lib%.a,%,$(LIBFILE))
LIBNAME += $(notdir $(SUBDIRS))

LIBS := $(foreach lib, $(LIBNAME), -l$(lib))
LIBS += -losal -lvb_mpi_arc_rtt -lfhses_mcu -laudio_codec_arc_rtt -lc -lm

LDFLAGS := $(foreach sensor, $(SENSOR_DATA), -Wl,-u,$(sensor))
LDFLAGS += $(LD_DIR)
LDFLAGS += -Wl,--gc-sections,-Map=$(MAP_FILE),-cref,-e,_reset -T link.ld
LDFLAGS += -Wl,--defsym=TextBase=$(TEXT_BASE) -Wl,-u,_close_r
LDFLAGS += -Wl,--start-group $(LIBS) -Wl,--end-group

# customize source code
CSRC:=$(SENSOR_PARAM_CSRC)
CSRC+=$(wildcard $(shell pwd)/components/fast_nn/*.c)
CSRC+=$(wildcard $(shell pwd)/components/rpc_customer/*.c)
CSRC+=$(wildcard $(shell pwd)/media/*.c)
CSRC+=$(wildcard $(shell pwd)/media/audio/src/*.c)
CSRC+=$(wildcard $(shell pwd)/media/dsp/src/*.c)
CSRC+=$(wildcard $(shell pwd)/media/isp/src/*.c)
CSRC+=$(wildcard $(shell pwd)/media/intelligent_det/md/*.c)
CSRC+=$(wildcard $(shell pwd)/media/intelligent_det/nna/*.c)
CSRC+=$(wildcard $(shell pwd)/media/overlay/*.c)
CSRC+=$(wildcard $(shell pwd)/media/audio/*.c)
COBJS=$(patsubst %.c, %.o, $(CSRC))

all: clean lib version $(TARGET)
	# @python3 ./mkimg.py 1 $(TARGET)
	./script/mkimg.sh 1 $(TARGET)

	cp rtthread_arc_$(SOC_ARCH).bin.img /code/images/g5k6_rtthread_arc.bin.img
	cp rtthread_arc_$(SOC_ARCH).bin.img ../../../../board_support/tools/flash_upgrade/flash_all/rtthread_arc.bin.img

# 自定义函数，生成$SUBDIRS目录下lib*.a
define create_lib_terget
$(LIB_DIR)/lib$(notdir $(1)).a: $(patsubst %.c,%.o,$(shell find $(1) -name "*.c"))
	@mkdir -p $(LIB_DIR)
	$$(CROSS_COMPILE)ar $(ARFLAGS) $$@ $$^
endef

# 调用执行函数create_lib_terget
$(foreach dir,$(SUBDIRS),$(eval $(call create_lib_terget,$(dir))))

lib: $(patsubst %,$(LIB_DIR)/lib%.a, $(notdir $(SUBDIRS)))

version:
	@echo "_sdk_git_version = 0x$(GIT_VERSION);" > fw_git_version.ld

distclean:
	@rm -f .config config.h

clean:
	@rm -f $(TARGET) $(ELF_FILE) $(MAP_FILE) $(DIS_FILE)
	@rm -f $(COBJS) $(TARGET).img
	@rm -f $(shell find $(TOPDIR) -name "*.o")
	@rm -rf $(LIB_DIR)

%_defconfig:
	@cp configs/$@ .config
	@python $(shell pwd)/script/menu_anno.py .config config.h $(CHIP_ID)

menuconfig:
	mconf Kconfig
	@python $(shell pwd)/script/menu_anno.py .config config.h $(CHIP_ID)
	@rm -f .config.old .config.cmd

%.o:%.c
	@echo build $(notdir $@)
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $< -o $@

$(TARGET): $(COBJS) $(ASMOBJS)
	@echo build $(ELF_FILE)
	$(CROSS_COMPILE)gcc -o $(ELF_FILE) $^ $(LDFLAGS)
	$(CROSS_COMPILE)objcopy -O binary $(ELF_FILE) $@
	$(CROSS_COMPILE)objdump -d $(ELF_FILE) > $(DIS_FILE)
	chmod 755 $(TOPDIR)/tool/fh_crc/crcappend
	$(TOPDIR)/tool/fh_crc/crcappend $@ $@.crc
	mv -f $@.crc $@

$(ASMOBJS):%.o:%.S
	$(CROSS_COMPILE)gcc $(ASFLAGS) -c $< -o $@
