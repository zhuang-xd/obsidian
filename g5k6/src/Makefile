include Rules.make

export CHIP_ID ?= MC632X
ifeq ($(CHIP_ID), MC632X)
LIBS = -ldbi -ldci  -ldsprpc_leader -lisprpc_leader -lpthread  -lrt -lvenc -lvmm_rpc -lrpc_customer -lacw_mpi -laudio_codec
else
LIBS = -ldbi  -ldsprpc_leader -lisprpc_leader -lpthread  -lrt -lacw_mpi -laudio_codec
endif
# CFLAGS +=  -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4
CFLAGS += -I$(shell pwd )/rpc_video/libpes/include
CFLAGS += -I$(shell pwd )/rpc_video/libdbi_over_tcp/include
CFLAGS += -I$(shell pwd )/rpc_video/librecord/include

CFLAGS += -DSENSOR=\"$(SENSOR)\"
# CFLAGS 	+= -O2

CXXFLAGS += $(CFLAGS)

OUT_DIR := $(SRC_HOME_DIR)/../out
BIN      := $(OUT_DIR)/g5k6
OBJS_DIR := $(OUT_DIR)/.objs

SRC_DIRS  :=  base \
			  entry \
              third_party/cJSON \
			  rpc_video/ \
			  rpc_video/libpes/src \
			  rpc_video/librecord/src \
			  eq_glasses/entry

CXXFLAGS += $(CFLAGS)

CLEANSUBDIRS := $(addsuffix .clean, $(SUBDIRS))

.PHONY: $(OBJS_DIR) $(SRC_DIRS) $(CLEANSUBDIRS)

SRCS      := $(wildcard $(addsuffix /*.c,$(SRC_DIRS)) $(addsuffix /*.cc,$(SRC_DIRS)))
OBJS      := $(patsubst %.c, $(OBJS_DIR)/%.o, $(patsubst %.cc, $(OBJS_DIR)/%.o, $(SRCS)))

all: $(BIN) install

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
	$(STRIP) $(BIN)

install:
	-mkdir -p $(INSTALL_DIR)
	$(STRIP) $(BIN)
	-cp -a $(BIN) $(INSTALL_DIR)

$(OBJS_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJS_DIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS_DIR) $(BIN)
