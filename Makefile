CCFLAGS= -std=gnu++11 -pedantic -Wall -ggdb3
TARGET = player ringmaster

TARGET: $(TARGET)

player: player.o networks.o
	g++ -o player $(CCFLAGS) player.o networks.o

ringmaster: ringmaster.o networks.o
	g++ -o ringmaster $(CCFLAGS) ringmaster.o networks.o

%.o: %.cpp networks.hpp httpParser.hpp response.hpp request.hpp potato.hpp
	g++ -c $(CCFLAGS) $<

clean:
	rm -f *.o  *~ player ringmaster 
