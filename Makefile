TARGET = siktacka-server siktacka-client
CC = clang++
CFLAGS = -Wall -Wextra --std=c++14 -O3
LFLAGS = -Wall -Wextra


HEADERS = common/HostAddress.hpp \
    common/utils.hpp \
    common/utils_impl.hpp \
    client/Client.hpp

COMMON_OBJS = common/HostAddress.o \
    common/utils.o
SERVER_OBJS = server/main.o \
    $(COMMON_OBJS)
CLIENT_OBJS = client/main.o \
    client/Client.o \
    $(COMMON_OBJS)

siktacka: $(TARGET)
all: siktacka

%.cpp: %.hpp

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
	scp -r * loli-citadel:students/sik/zad2/ >/dev/null
	ssh loli-citadel "(cd students/sik/zad2/; make)"
