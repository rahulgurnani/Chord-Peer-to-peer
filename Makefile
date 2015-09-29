peer: peer.cpp node.o
	g++ -Wall -o peer node.o peer.cpp

node.o: node.cpp
	g++ -Wall -c node.cpp

