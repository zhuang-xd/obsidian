include ../Rules.make

LIBS = -lacw_mpi -lpthread

ifeq ($(DEBUG),0)
  CFLAGS 	+= -O2
else
  CFLAGS 	+= -O0 -g3
endif

BIN = mcu_control

SRCS = mcu_control.cc
OBJS	= $(SRCS:%.cc=%.o)
DEPENDS	= $(subst .o,.d,$(OBJS))

all: $(BIN) install

$(BIN): $(OBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
ifeq ($(DEBUG),0)
	$(STRIP) $(BIN)
endif

install:
	-mkdir -p $(INSTALL_DIR)
	-cp -a $(BIN) $(INSTALL_DIR)
	-cp -a $(BIN) ~/code/yunqu

clean:
	rm -f $(BIN)
	rm -f *.o *.d
	rm -f ../common/*.o ../common/*.d

ifneq "$(MAKECMDGOALS)" "clean"
  -include $(DEPENDS)
endif

%.o: %.cc
	$(CXX) $(CFLAGS) -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"

.PHONY: clean
