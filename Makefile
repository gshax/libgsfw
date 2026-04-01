CFLAGS = -Wall -O2 -Wno-unused-variable -I.
LDFLAGS = -static

OUTDIR ?= build/native
ODIR = $(OUTDIR)/obj
BDIR = $(OUTDIR)

_TARGETS = imgtool fwtool
TARGETS = $(patsubst %,$(BDIR)/%,$(_TARGETS))

# firmware library objects
_LIBGSFW_OBJS = util firmware/shared firmware/crypto firmware/aes \
    firmware/family_defs firmware/ht8xx_dvf101 firmware/ht8xx firmware/htlegacy
LIBGSFW_OBJS = $(patsubst %,$(ODIR)/%.o,$(_LIBGSFW_OBJS))
LIBGSFW = $(BDIR)/libgsfw.a

_OBJ = $(_LIBGSFW_OBJS) $(_TARGETS)
OBJ = $(patsubst %,$(ODIR)/%.o,$(_OBJ))

all: $(TARGETS)

# firmware library
$(LIBGSFW): $(LIBGSFW_OBJS)
	$(AR) rcs $@ $^

# firmware tools
$(BDIR)/imgtool: $(ODIR)/imgtool.o $(LIBGSFW)
$(BDIR)/fwtool:  $(ODIR)/fwtool.o  $(LIBGSFW)

$(ODIR)/%.o: src/%.c
	@mkdir -p $(ODIR)
	@mkdir -p $(ODIR)/firmware
	@mkdir -p $(ODIR)/driver
	$(CC) -c -o $@ $< $(CFLAGS)

$(BDIR)/%: $(ODIR)/%.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f $(OBJ) $(LIBGSFW)
	rm -f $(TARGETS)
	rmdir $(ODIR)/firmware
	rmdir $(ODIR)/driver
	rmdir $(ODIR)
	rmdir $(BDIR)

# shorthand targets
.PHONY: $(_TARGETS) libgsfw
$(_TARGETS): %: $(BDIR)/%
libgsfw: $(LIBGSFW)

.PHONY: clean
