# Change these lines before running `make tar`
FIRST_NAME=Alejandro
LAST_NAME=Sandoval
KUID=3012949

LAB=07
TAR_BASENAME=Lab$(LAB)_$(FIRST_NAME)_$(LAST_NAME)_$(KUID)
DELIVERABLES=./examples ./src examples.pl

PROGNAME = simulator

CC = gcc --std=gnu11
CFLAGS = -Wall -g


####################################################################
#                           IMPORTANT                              #
####################################################################
# Add files to their respective line to get this makefile to build #
# them.                                                            #
####################################################################
# NOTE: The submission scripts assume all files in `CFILELIST` end with
# .c and all files in `HFILES` end in .h
CFILELIST = simulator.c libscheduler/libscheduler.c libpriqueue/libpriqueue.c
HFILELIST = libscheduler/libscheduler.h libpriqueue/libpriqueue.h

# Add libraries that need linked as needed (e.g. -lm -lpthread)
LIBLIST =

# Include locations
INCLIST = ./src ./src/libscheduler ./src/libpriqueue

# Doxygen configuration file
DOXYGENCONF = ./doc/Doxyfile

####################################################################

SRCDIR = ./src/
OBJDIR = ./obj/

EXECNAME = $(patsubst %,./%,$(PROGNAME))

CFILES = $(patsubst %,$(SRCDIR)%,$(CFILELIST))
HFILES = $(patsubst %,$(SRCDIR)%,$(HFILELIST))
OFILES = $(patsubst %.c,$(OBJDIR)%.o,$(CFILELIST))

RAWC = $(patsubst %.c,%,$(addprefix $(SRCDIR), $(CFILELIST)))
RAWH = $(patsubst %.h,%,$(addprefix $(SRCDIR), $(HFILELIST)))

INCDIRS = $(patsubst %,-I%,$(INCLIST))

SUBMISSION = $(TAR_BASENAME)

OBJINNERDIRS = $(patsubst $(SRCDIR)%,$(OBJDIR)%,$(shell find $(SRCDIR) -type d))
SUBMISSIONDIRS = $(addprefix $(SUBMISSION)/,$(shell find $(SRCDIR) -type d))

# Build the the simulator & queuetest executables
all: $(PROGNAME) queuetest

# Build the object directories
$(OBJINNERDIRS):
	$(foreach dir, $(OBJINNERDIRS), mkdir -p $(dir);)

# Build the program
$(PROGNAME): $(OBJINNERDIRS) $(PROGNAME)-inner
$(PROGNAME)-inner: $(OFILES)
	$(CC) $(CFLAGS) $^ -o $(PROGNAME) $(LIBLIST)


# Generic build target for all compilation units. NOTE: Changing a
# header requires you to rebuild the entire lab
$(OBJDIR)%.o: $(SRCDIR)%.c $(HFILES)
	$(CC) $(CFLAGS) -c $(INCDIRS) -o $@ $< $(LIBS)

# Build a testing harness for the priority queue
queuetest: $(OBJINNERDIRS) queuetest-inner
queuetest-inner: ./src/queuetest.c ./src/libpriqueue/libpriqueue.o
	$(CC) $(CFLAGS) $^ -o queuetest $(LIBLIST)

# Build and run the program
test: all
#	./queuetest
	./examples.pl

# Build the documentation
doc: $(DOXYGENCONF) $(CFILES)
	doxygen $(DOXYGENCONF)

tar: clean
#	create temp dir
	mkdir $(TAR_BASENAME)
#	copy the necessary files into the temp dir
	cp -r $(DELIVERABLES) Makefile $(TAR_BASENAME)
#	create the submission tar.gz
	tar cvzf $(TAR_BASENAME).tar.gz $(TAR_BASENAME)
#	remove the temp dir
	rm -rf $(TAR_BASENAME)

# Remove all generated files and directories
clean:
	-rm -rf $(PROGNAME) queuetest obj *~ $(SUBMISSION)* doc/html

.PHONY: all test tar doc clean
