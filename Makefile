TARGET = siktacka-server siktacka-client
EXT_LIBS = -lz
CC = g++
CFLAGS = -Wall -Wextra -Wpedantic --std=c++14 -O3 -I.
LFLAGS = -Wall -Wextra $(EXT_LIBS)


HEADERS = \
	common/utils.hpp \
	common/RandomNumberGenerator.hpp \
	common/network/HostAddress.hpp \
	common/network/Socket.hpp \
	common/network/TcpSocket.hpp \
	common/network/UdpSocket.hpp \
	common/protocol/GameEvent.hpp \
	common/protocol/HeartBeat.hpp \
	common/protocol/MultipleGameEvent.hpp \
	common/protocol/utils.hpp \
	client/Client.hpp \
	server/Server.hpp

COMMON_OBJS = \
	common/RandomNumberGenerator.o \
	common/network/HostAddress.o \
	common/network/Socket.o \
	common/network/TcpSocket.o \
	common/network/UdpSocket.o \
	common/protocol/GameEvent.o \
	common/protocol/HeartBeat.o \
	common/protocol/MultipleGameEvent.o \
	common/protocol/utils.o

SERVER_OBJS = \
	server/main.o \
	server/Server.o \
	$(COMMON_OBJS)

CLIENT_OBJS = \
	client/main.o \
	client/Client.o \
	$(COMMON_OBJS)

all: siktacka
siktacka: $(TARGET)

%.o: %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

siktacka-server: $(SERVER_OBJS)
	$(CC) $(LFLAGS) $^ -o $@

siktacka-client: $(CLIENT_OBJS)
	$(CC) $(LFLAGS) $^ -o $@


.PHONY: clean remote
clean:
	rm -f $(TARGET) $(CLIENT_OBJS) $(SERVER_OBJS)

remote:
	scp -r * '${REMOTE_HOST}':'${REMOTE_DIR}' >/dev/null
	ssh '${REMOTE_HOST}' "(cd '${REMOTE_DIR}'; make)"
