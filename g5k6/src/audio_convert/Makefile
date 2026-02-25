include ../Rules.make

LIBS = -laudio_codec -lm

ifeq ($(DEBUG),0)
  CFLAGS    += -O2
else
  CFLAGS    += -O0 -g3
endif

BIN = audio_convert

SRCS = $(wildcard *.c)
OBJS    = $(SRCS:%.c=%.o)
DEPENDS = $(subst .o,.d,$(OBJS))

all: $(BIN) install

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

install:
	mkdir -p $(INSTALL_DIR)
	-cp -a $(BIN) $(INSTALL_DIR)

clean:
	rm -f $(BIN)
	rm -f *.o *.d
	rm -f ../common/*.o ../common/*.d

ifneq "$(MAKECMDGOALS)" "clean"
  -include $(DEPENDS)
endif

%.o: %.c
	$(CC) $(CFLAGS) -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"

.PHONY: clean