DEBUG ?= 0
STATIC ?= 1

SRC_HOME_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

-include $(SRC_HOME_DIR)/../../config.mk

CC          := $(CROSS_COMPILE)gcc
CXX         := $(CROSS_COMPILE)g++
AS          := $(CROSS_COMPILE)as
LD          := $(CROSS_COMPILE)ld
AR          := $(CROSS_COMPILE)ar
NM          := $(CROSS_COMPILE)nm
STRIP       := $(CROSS_COMPILE)strip
RANLIB      := $(CROSS_COMPILE)ranlib
OBJDUMP     := $(CROSS_COMPILE)objdump
OBJCOPY     := $(CROSS_COMPILE)objcopy
ROOTDIR := $(SRC_HOME_DIR)

SDK_INC_DIR = $(SRC_HOME_DIR)/../include
SAMPLE_COMMON_INC_DIR = $(SRC_HOME_DIR)/common
LIB_PATH = $(SRC_HOME_DIR)/../lib

ifeq ($(STATIC),1)
SDK_LIB_DIR = $(LIB_PATH)/static
else
SDK_LIB_DIR = $(LIB_PATH)/dynamic
endif

INCLUDES = -I$(SDK_INC_DIR) -I$(SAMPLE_COMMON_INC_DIR) -I$(SAMPLE_COMMON_INC_DIR)/video_config -I$(ROOTDIR)

CFLAGS = -Wall $(INCLUDES)
CXXFLAGS := -Wall $(INCLUDES) --std=c++14
LDFLAGS = -L$(SDK_LIB_DIR)
SDK_LIBS = -ldbi -ldci -ldsp -lisp -lispcore -ladvapi -lvmm

INSTALL_DIR ?= $(SRC_HOME_DIR)/../bin

SENSOR ?= jxf22_mipi

ifeq ($(SENSOR),imx138)
SENSOR_LIB = -limx-138
else
SENSOR_LIB = -l$(SENSOR)
endif
CFLAGS += -DSENSOR=\"$(SENSOR)\"
SDK_LIBS += $(SENSOR_LIB)
CFLAGS += -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4 -fno-aggressive-loop-optimizations -z noexecstack -O2
