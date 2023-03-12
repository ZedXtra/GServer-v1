OBJS = cfg.o iostream.h gserver.o levels.o otherstuff.o mysql.o pascalstrings.o convertedclasses.o passwords.o colours.h
SLFLAGS = -L/usr/lib/mysql -lz -lmysqlclient -lstdc++ -lcrypto -lm
CC = g++ -g -std=gnu++17 -I/usr/include/mysql/ -Wno-deprecated -fpermissive
LD = g++

.SUFFIXES: .cpp
.cpp.o:
	$(CC) -c $<

all: gserver

gserver: $(OBJS) 
	$(LD) -fpermissive -o ./gserver $(OBJS) $(SLFLAGS)

clean:
	-rm *.o
