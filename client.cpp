/**
 * Chat Client for Windows
 * 
 * Simple client that connects to the chat server.
 * Uses Winsock2 for networking and console I/O.
 */

#include "sockutil.h"
#include <iostream>
#include <string>
#include <atomic>
#include <conio.h>
#include "win32_compat.h"

// Global state
SOCKET g_socket = INVALID_SOCKET;
std::atomic<bool> g_running{true};
std::string g_username;

// Colors for messages
void SetConsoleColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void ResetConsoleColor() {
    SetConsoleColor(7); // Default white
}

void PrintMessage(const std::string& msg, int color = 7) {
    SetConsoleColor(color);
    std::cout << msg;
    ResetConsoleColor();
    std::cout << std::flush;
}

// Receive thread
void ReceiveMessages() {
    char buffer[MAX_LEN];
    
    while (g_running) {
        ZeroMemory(buffer, MAX_LEN);
        int bytes = recv(g_socket, buffer, MAX_LEN - 1, 0);
        
        if (bytes <= 0) {
            if (g_running) {
                PrintMessage("\nDisconnected from server.\n", 12); // Red
                g_running = false;
            }
            break;
        }
        
        std::string message(buffer, bytes);
        
        // Color code based on message type
        if (message.find("has joined") != std::string::npos ||
            message.find("has left") != std::string::npos) {
            PrintMessage(message, 14); // Yellow
        }
        else if (message.find("[Whisper") != std::string::npos) {
            PrintMessage(message, 13); // Magenta
        }
        else if (message.find("Available") != std::string::npos ||
                 message.find("Online users") != std::string::npos ||
                 message.find("commands:") != std::string::npos) {
            PrintMessage(message, 11); // Cyan
        }
        else if (message.find("Error") != std::string::npos ||
                 message.find("Failed") != std::string::npos ||
                 message.find("kicked") != std::string::npos ||
                 message.find("banned") != std::string::npos ||
                 message.find("muted") != std::string::npos) {
            PrintMessage(message, 12); // Red
        }
        else {
            PrintMessage(message, 10); // Green
        }
    }
}

// Send thread
void SendMessages() {
    std::string input;
    
    while (g_running) {
        std::getline(std::cin, input);
        
        if (!g_running) break;
        
        if (input.empty()) continue;
        
        // Send to server
        int result = send(g_socket, input.c_str(), (int)input.length(), 0);
        if (result == SOCKET_ERROR) {
            PrintMessage("Failed to send message.\n", 12);
            g_running = false;
            break;
        }
        
        // Check for exit
        if (input == "#exit") {
            g_running = false;
            break;
        }
        
        // Echo own message
        if (input[0] != '#') {
            PrintMessage("You: " + input + "\n", 15); // Bright white
        }
    }
}

// Signal handler
BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT) {
        if (g_socket != INVALID_SOCKET) {
            std::string exit_cmd = "#exit";
            send(g_socket, exit_cmd.c_str(), (int)exit_cmd.length(), 0);
        }
        g_running = false;
        return TRUE;
    }
    return FALSE;
}

int main(int argc, char* argv[]) {
    // Parse arguments
    const char* server_ip = "127.0.0.1";
    int port = 8080;
    
    if (argc >= 2) {
        server_ip = argv[1];
    }
    if (argc >= 3) {
        port = atoi(argv[2]);
    }
    
    // Enable ANSI colors on Windows 10+
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    
    std::cout << "========================================\n";
    std::cout << "       Chat Client v2.0 (Windows)       \n";
    std::cout << "========================================\n\n";
    
    // Initialize Winsock
    if (!InitializeWinsock()) {
        std::cerr << "Failed to initialize Winsock" << std::endl;
        return 1;
    }
    
    // Set up signal handler
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
    
    // Connect to server
    std::cout << "Connecting to " << server_ip << ":" << port << "...\n";
    
    g_socket = CreateClientSocket(server_ip, port);
    if (g_socket == INVALID_SOCKET) {
        std::cerr << "Failed to connect to server" << std::endl;
        CleanupWinsock();
        return 1;
    }
    
    PrintMessage("Connected!\n\n", 10);
    
    // Get username
    std::cout << "Enter your username: ";
    std::getline(std::cin, g_username);
    
    if (g_username.empty()) {
        g_username = "Anonymous";
    }
    
    // Send username to server
    send(g_socket, g_username.c_str(), (int)g_username.length(), 0);
    
    PrintMessage("\nWelcome, " + g_username + "!\n", 14);
    PrintMessage("Type #help for available commands. Type messages and press Enter to send.\n\n", 11);
    
    // Start receive thread
    w32::Thread recv_thread(ReceiveMessages);
    
    // Send messages in main thread
    SendMessages();
    
    // Cleanup
    g_running = false;
    
    if (g_socket != INVALID_SOCKET) {
        shutdown(g_socket, SD_BOTH);
        closesocket(g_socket);
    }
    
    if (recv_thread.joinable()) {
        recv_thread.join();
    }
    
    CleanupWinsock();
    
    PrintMessage("\nGoodbye!\n", 14);
    return 0;
}