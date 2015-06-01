CFLAGS = -Wall -pedantic -std=c11 -DNF_PLAT_POSIX -DNF_PRINTF -Wno-unused-function
LDFLAGS = -lreadline
CC = clang

SRCDIR = ./src
OBJDIR = ./build
TESTDIR = ./tests

INCLUDES = $(SRCDIR)/nf_common.h

OBJS = \
	$(OBJDIR)/nf_stmt.o \
	$(OBJDIR)/nf_intp.o \
	$(OBJDIR)/nf_lex.o \
	$(OBJDIR)/nf_mach.o \
	$(OBJDIR)/nf_base.o \
	$(OBJDIR)/nf_word.o \
	$(OBJDIR)/nf_snprintf.o \
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
	rm -f $(OBJDIR)/*.o $(OBJDIR)/posix/*.o

test: $(NF)
	$(TESTDIR)/tests.py
