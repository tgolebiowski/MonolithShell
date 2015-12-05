#build my shell

#Defining the Compiler
CC = gcc

#Compiler Flags
CFLAGS = -Wall -g

#Library file Locations
LFLAGS = -LLibs

#Librarys to Use
LIBS = -lGdi32 

#source files
SRCS = TomShell.c

#object files
OBJS = $(SRCS:.c=.o)

#exe file name
MAIN = TomShell

all: $(MAIN)
	@echo TomShell compilation complete.

$(MAIN) : $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	$(RM) *.o *~ $(MAIN)
