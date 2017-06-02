TARGET = siktacka-server siktacka-client
CC = clang++
CFLAGS = -Wall -Wextra -O3
LFLAGS = -Wall -Wextra


SERVER_OBJS = server/main.cpp
CLIENT_OBJS = client/main.cpp


siktacka: $(TARGET)
all: siktacka

%.cpp: %.hpp

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

siktacka-server: $(SERVER_OBJS)
	$(CC) $(LFLAGS) $^ -o $@

siktacka-client: $(CLIENT_OBJS)
	$(CC) $(LFLAGS) $^ -o $@


.PHONY: clean remote
clean:
	rm -f $(TARGET) *.o *~ *.bak

remote:
	scp -r * loli-citadel:students/sik/zad2/
	ssh loli-citadel "(cd students/sik/zad2/; make)"
