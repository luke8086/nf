CFLAGS = -Wall -pedantic -std=c11 -DNF_PLAT_POSIX -DNF_SUPPORTS_LONG_LONG -Wno-unused-function
LDFLAGS = -lreadline
CC = clang

SRCDIR = ./src
OBJDIR = ./build
TESTDIR = ./tests

INCLUDES = $(SRCDIR)/nf_cmmn.h

OBJS = \
	$(OBJDIR)/nf_stmt.o \
	$(OBJDIR)/nf_intp.o \
	$(OBJDIR)/nf_lex.o \
	$(OBJDIR)/nf_mach.o \
	$(OBJDIR)/nf_base.o \
	$(OBJDIR)/nf_word.o \
	$(OBJDIR)/nf_prtf.o \
	$(OBJDIR)/nf_str.o \
	$(OBJDIR)/posix/nf_main.o \
	$(OBJDIR)/posix/nf_libc.o \

NF = $(OBJDIR)/nf

all: $(NF)

$(OBJDIR)/posix/%.o: $(SRCDIR)/posix/%.c $(INCLUDES)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCLUDES)
	$(CC) $(CFLAGS) -c $< -o $@

$(NF): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(NF)

clean:
	rm -f $(OBJDIR)/*.o $(OBJDIR)/posix/*.o $(OBJDIR)/*.img

test: $(NF)
	$(TESTDIR)/tests.py

# Launch the x86 version in qemu, unrelated to all the code above
qemu:
	cat $(OBJDIR)/NF_BOOT.BIN $(OBJDIR)/NF_BOOT.BIN $(OBJDIR)/NF.COM > $(OBJDIR)/nf_x86.img
	qemu-system-i386 -drive format=raw,file=$(OBJDIR)/nf_x86.img

