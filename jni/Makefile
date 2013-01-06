
SRC := freedv_cli.c freedv_usb.c freedv_decode.c \
	freedv/codebookge.c freedv/codebook.c freedv/kiss_fft.c freedv/nlp.c \
	freedv/interp.c freedv/fdmdv.c freedv/sine.c freedv/codec2.c \
	freedv/dump.c freedv/codebookdt.c freedv/freedv_process.c \
	freedv/pack.c freedv/codebookd.c \
	freedv/codebookjnd.c freedv/postfilter.c freedv/lsp.c \
	freedv/codebookvq.c freedv/codebookjvm.c freedv/quantise.c \
	freedv/lpc.c freedv/phase.c freedv/codebookvqanssi.c

OBJ := $(SRC:.c=.o)

LIBUSB_CFLAGS := $(shell pkg-config --cflags libusb-1.0)
LIBUSB_LDLIBS := $(shell pkg-config --libs libusb-1.0)

CFLAGS += -O0 -Wall -g -Wno-unused-variable -Wno-unused-but-set-variable -pthread $(LIBUSB_CFLAGS)
LDLIBS += $(LIBUSB_LDLIBS) -lm
LDFLAGS += -pthread $(LDLIBS)

PROGRAM := freedv_cli

all: $(PROGRAM)

$(PROGRAM): $(OBJ)
	$(LINK.c) $^ $> $(OUTPUT_OPTIONS) -o $@

.PHONY: all clean

clean:
	$(RM) $(OBJ) $(PROGRAM)
