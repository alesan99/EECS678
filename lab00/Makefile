# Define the compiler
CC = gcc

# Define any compile-time flags
CFLAGS = -Wall -g

# Define any directories containing header files
INCLUDES = -I. -ITests

# Define library paths in addition to /usr/lib
LFLAGS =

# Define any libraries to link into executable
LIBS =

# Define the C source files
SRCS = main.c

# Define the C header files (included in the compilation by the .c files)
HEADERS = LinkedList.h ListQuestions.h Tests/test1.h

# Define the executable file
MAIN = runTests

.PHONY: clean

all: $(MAIN)
	@echo Program has been compiled!

$(MAIN): $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(SRCS) $(LFLAGS) $(LIBS)

clean:
	$(RM) *.o *~ $(MAIN)

