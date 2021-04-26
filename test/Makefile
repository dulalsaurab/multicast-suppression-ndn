CC = g++
PYLIBPATH = $(python-config --exec-prefix)/lib
LIB = -L$(PYLIBPATH) $(python-config --libs) -lboost_python
OPTS = $(python-config --include) -O2
INCLUDES =  -I/usr/include/python3.8

default: zoo.so
	@python ./visit_zoo.py

zoo.so: zoo.o
	$(CC) $(LIB) $(INCLUDES) -fpic -Wl, -rpath, $(PYLIBPATH) -shared $< -o $@

zoo.o: zoo.cpp Makefile
	$(CC) $(OPTS) $(INCLUDES) -fPIC -c $< -o $@

clean:
	rm -rf *.so *.o

.PHONY: default clean
