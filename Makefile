build: server client

server: Card.cpp Card.h ClientTerminal.cpp ClientTerminal.h Server.cpp Server.h server_main.cpp
	g++ -o server Card.cpp Card.h ClientTerminal.cpp ClientTerminal.h Server.cpp Server.h server_main.cpp

client: client_main.cpp
	g++ -o client client_main.cpp

clean:
	rm client
	rm server
