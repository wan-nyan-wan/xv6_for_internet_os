PROGNAME = 9psv
STYX2000_DIR = $U/$(PROGNAME)
STYX2000_SRCS = \
	$(STYX2000_DIR)/server.c \
	$(STYX2000_DIR)/util.c \
	$(STYX2000_DIR)/fid.c \
	$(STYX2000_DIR)/file.c \
	$(STYX2000_DIR)/qid.c \
	$(STYX2000_DIR)/req.c \
	$(STYX2000_DIR)/version.c \
	$(STYX2000_DIR)/error.c \
	$(STYX2000_DIR)/attach.c \
	$(STYX2000_DIR)/walk.c \
	$(STYX2000_DIR)/open.c \
	$(STYX2000_DIR)/read.c \
	$(STYX2000_DIR)/clunk.c \
	$(STYX2000_DIR)/remove.c \
	$(STYX2000_DIR)/stat.c \

STYX2000_OBJS = $(addprefix $(BUILD_DIR)/, $(STYX2000_SRCS:.c=.o))
STYX2000_DEPS = $(STYX2000_OBJS:.o=.d)

-include $(STYX2000_DEPS)

_$(PROGNAME): $(STYX2000_OBJS) $(ULIBOBJS)
	@mkdir -p $(BUILD_DIR)/$U/$(PROGNAME)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $(BUILD_DIR)/$U/$@ $^