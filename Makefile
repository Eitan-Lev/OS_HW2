# Makefile for the HW2
CC = g++
CFLAGS = -g -Wall -lpthread 
CCLINK = $(CC)
OBJS = Bank.o bankAccount.o bankMap.o Exceptions.o MacroBank.h
RM = rm -f
# Creating the  executable
Bank: $(OBJS) 
	$(CCLINK) $(CFLAGS) -o Bank $(OBJS)
# Creating the object files
bankMap.o: bankMap.cpp bankMap.h bankAccount.h 
bankAccount.o: bankAccount.cpp bankAccount.h
Bank.o: Bank.cpp bankMap.h MacroBank.h
Exceptions.o: Exceptions.cpp Exceptions.h
MacroBank.o : MacroBank.h
# Cleaning old files before new make
clean: 
	$(RM) Bank *.o *~ "#"* core.*


