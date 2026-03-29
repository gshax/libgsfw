CFLAGS = -Wall -O2 -Wno-unused-variable
LDFLAGS = -static

OUTDIR ?= native
ODIR = $(OUTDIR)/obj
BDIR = $(OUTDIR)

_OBJ = hdr util imgtool fwtool fwmkhdr
OBJ = $(patsubst %,$(ODIR)/%.o,$(_OBJ))

_TARGETS = imgtool fwtool fwmkhdr
TARGETS = $(patsubst %,$(BDIR)/%,$(_TARGETS))

all: $(TARGETS)

$(BDIR)/imgtool: $(ODIR)/imgtool.o $(ODIR)/util.o $(ODIR)/hdr.o
$(BDIR)/fwtool: $(ODIR)/fwtool.o $(ODIR)/util.o $(ODIR)/hdr.o
$(BDIR)/fwmkhdr: $(ODIR)/fwmkhdr.o $(ODIR)/util.o $(ODIR)/hdr.o

$(ODIR)/%.o: %.c
	@mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BDIR)/%: $(ODIR)/%.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f $(OBJ)
	rm -f $(TARGETS)
	rmdir $(ODIR)
	rmdir $(BDIR)

.PHONY: clean