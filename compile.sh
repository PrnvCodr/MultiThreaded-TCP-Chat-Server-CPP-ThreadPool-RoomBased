#!/bin/bash

# Compile the client and server with ncurses
g++ -o ser server.cpp sockutil.cpp -lncurses
g++ -o cli client.cpp sockutil.cpp -lncurses

echo "Compilation complete!"
echo "Run the server with: ./ser <port>"
echo "Run the client with: ./cli <server_ip> <port>"