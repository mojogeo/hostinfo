INCLUDE := -I./include -I/usr/local/include/
CC      := g++
CFLAGS  := $(INCLUDE) -pthread -static-libstdc++ -static-libgcc
ODIR    := .
LIBS    := -lm -ldl /usr/local/lib64/libcpprest.a /usr/local/lib64/libboost_system.a /usr/local/lib64/libboost_filesystem.a -lssl -lcrypto

_DEPS = argh.h RestIF.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = hostinfo.o RestIF.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.cpp
	mkdir -p $(ODIR)
	$(CC) $(CFLAGS) -c -o $@ $< 

hostinfo: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -fr hostinfo *~ core include/*~ *.o tmp/
