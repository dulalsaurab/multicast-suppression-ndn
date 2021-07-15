CXXFLAGS  =-std=c++14 $(shell pkg-config --cflags libndn-cxx)
LDFLAGS =-std=c++14 $(shell pkg-config --libs libndn-cxx)
CC = $(CXX)

app: consumer producer

consumer: consumer.o

server: producer.o

clean:
	rm -Rf consumer producer *.o
