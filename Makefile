TARGET = siktacka-server siktacka-client
EXT_LIBS = -lz
CC = clang++
CFLAGS = -Wall -Wextra -Wpedantic --std=c++14 -O3 -I.
LFLAGS = -Wall -Wextra $(EXT_LIBS)


HEADERS = common/network/HostAddress.hpp \
    common/utils.hpp \
    common/utils_impl.hpp \
    common/network/Socket.hpp \
    common/network/UdpSocket.hpp \
    common/network/TcpSocket.hpp \
    common/protocol/HeartBeat.hpp \
    common/protocol/GameEvent.hpp \
    common/protocol/utils.hpp \
    client/Client.hpp

COMMON_OBJS = common/network/HostAddress.o \
    common/utils.o \
    common/network/Socket.o \
    common/network/UdpSocket.o \
    common/network/TcpSocket.o \
    common/protocol/HeartBeat.o \
    common/protocol/GameEvent.o \
    common/protocol/utils.o

SERVER_OBJS = server/main.o \
    $(COMMON_OBJS)

CLIENT_OBJS = client/main.o \
    client/Client.o \
    $(COMMON_OBJS)

siktacka: $(TARGET)
all: siktacka

%.o: %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

siktacka-server: $(SERVER_OBJS)
	$(CC) $(LFLAGS) $^ -o $@

siktacka-client: $(CLIENT_OBJS)
	$(CC) $(LFLAGS) $^ -o $@


.PHONY: clean remote
clean:
	rm -f $(TARGET) *.o *~ *.bak

remote:
	scp -r * '${REMOTE_HOST}':'${REMOTE_DIR}' >/dev/null
	ssh '${REMOTE_HOST}' "(cd '${REMOTE_DIR}'; make)"
