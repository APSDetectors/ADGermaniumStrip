# the compiler: gcc for C program, define as g++ for C++
CC = g++

ZMQ = /home/beams0/TMADDEN/EPICS/ADEpics/synApps_5_5/support/areaDetector-Master/ZMQ


# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall -I$(ZMQ)/include -L$(ZMQ)/lib -lzmq



# the build target executable:
SRV = helloworldsrv
CLT = helloworldclt
MAIASIM=maia_simserver
MAIACLT=maia_client

all: $(SRV) $(CLT) $(MAIASIM) $(MAIACLT)

$(SRV): $(SRV).cpp
	$(CC) $(CFLAGS) -o $(SRV) $(SRV).cpp


$(CLT): $(CLT).cpp
	$(CC) $(CFLAGS) -o $(CLT) $(CLT).cpp

  
$(MAIASIM): $(MAIASIM).cpp erfmath.cpp stopWatch.cpp
	$(CC) $(CFLAGS) -o $(MAIASIM) erfmath.cpp stopWatch.cpp $(MAIASIM).cpp
  
$(MAIACLT): $(MAIACLT).cpp 
	$(CC) $(CFLAGS) -o $(MAIACLT) $(MAIACLT).cpp

clean:
	$(RM) $(SRV) $(CLT) $(MAIASIM) $(MAIACLT)