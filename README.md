# High-Performance Multi-Threaded Chat Server v2.0

A highly optimized, low-latency multi-threaded chat server built in C++ for **Windows** using:

- **IOCP (I/O Completion Ports)** - Windows' most efficient async I/O mechanism
- **Thread Pool** - Fixed worker threads to prevent thread thrashing
- **Task Queue** - Events are queued and processed by idle workers
- **Rate Limiting** - Connection and message rate limiting
- **Chat Rooms** - Multiple named channels
- **Message Persistence** - Chat history saved to files

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     IOCP Completion Port                     │
└─────────────────┬───────────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────────────┐
│                      Thread Pool (N workers)                 │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐           │
│  │ Worker 1│ │ Worker 2│ │ Worker 3│ │ Worker N│           │
│  └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘           │
└───────┼───────────┼───────────┼───────────┼─────────────────┘
        │           │           │           │
        └───────────┴─────┬─────┴───────────┘
                          │
                          ▼
        ┌────────────────────────────────────┐
        │           Task Queue               │
        │  ┌──────┐ ┌──────┐ ┌──────┐       │
        │  │ Task │→│ Task │→│ Task │→ ...  │
        │  └──────┘ └──────┘ └──────┘       │
        └────────────────────────────────────┘
```

## Features

### Core Features
- **Thread Pool**: Fixed number of worker threads (default: CPU cores)
- **Task Queue**: Lock-free enqueueing, workers pick up tasks from queue
- **IOCP**: Windows native high-performance async I/O
- **Connection Rate Limiting**: Prevents DoS attacks (default: 50 conn/sec)
- **Message Rate Limiting**: Anti-spam protection (default: 60 msg/min)

### Chat Features
- **Multiple Chat Rooms**: #general (default), create custom rooms
- **Private Messaging**: Whisper directly to users
- **Message History**: Persisted to disk, retrievable via #history
- **User Presence**: See who's online, what room they're in
- **Admin Commands**: Kick, ban, mute users

### Technical Details
- C++17 standard
- Winsock2 for networking
- No external dependencies (pure Windows API)
- Thread-safe data structures with mutexes
- Graceful shutdown handling

## Building

### Prerequisites
- Windows 10 or later
- Visual Studio 2019+ OR MinGW-w64

### Using Visual Studio (Recommended)

1. Open "Developer Command Prompt for VS"
2. Navigate to project directory
3. Run the build script:
```batch
build.bat
```

### Using MinGW-w64

1. Ensure g++ is in PATH
2. Run:
```batch
build_mingw.bat
```

### Using CMake

```batch
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

## Running

### Start the Server

```batch
build\server.exe 8080
```

You'll see:
```
========================================
  High-Performance Chat Server v2.0    
  Windows IOCP + Thread Pool Edition   
========================================

[12:34:56] Initializing components...
[12:34:56] Thread pool created with 8 workers
[12:34:56] Connection manager initialized
[12:34:56] Chat room manager initialized (default room: #general)
[12:34:56] Message store initialized
[12:34:56] Server listening on port 8080
```

### Start a Client

```batch
build\client.exe 127.0.0.1 8080
```

You'll be prompted for a username, then you can start chatting!

## Client Commands

| Command | Description |
|---------|-------------|
| `#help` | Show available commands |
| `#rooms` | List all chat rooms |
| `#join <room>` | Join a room |
| `#create <room>` | Create a new room |
| `#leave` | Leave current room (go to #general) |
| `#online` | List all online users |
| `#whisper <user> <msg>` | Send private message |
| `#history [n]` | Show last n messages (default 10) |
| `#exit` | Disconnect from server |

### Admin Commands
| Command | Description |
|---------|-------------|
| `#kick <user>` | Kick user from server |
| `#ban <user>` | Ban user's IP address |
| `#mute <user> [seconds]` | Mute user (default 60s) |

## Project Structure

```
├── server.cpp           # Main server application
├── client.cpp           # Chat client
├── thread_pool.h/cpp    # Thread pool implementation
├── iocp_server.h/cpp    # IOCP wrapper and event handling
├── sockutil.h/cpp       # Windows socket utilities
├── connection_manager.h/cpp  # Rate limiting, banning
├── chat_room.h/cpp      # Room management
├── message_store.h/cpp  # Message persistence
├── CMakeLists.txt       # CMake build file
├── build.bat            # MSVC build script
├── build_mingw.bat      # MinGW build script
└── README.md            # This file
```

## Configuration

Edit these constants in `server.cpp`:

```cpp
constexpr size_t THREAD_POOL_SIZE = 0;  // 0 = auto (CPU cores)

// Connection Manager Config
conn_config.max_connections_per_second = 50;
conn_config.max_messages_per_minute = 60;
conn_config.connection_timeout_seconds = 300;
conn_config.max_total_connections = 1000;

// Message Store Config
store_config.max_messages_per_room = 100;
store_config.log_directory = "./chat_logs";
```

## Performance

The Thread Pool + IOCP architecture provides:

- **No thread thrashing**: Fixed thread count regardless of connections
- **Efficient I/O**: IOCP handles thousands of connections efficiently
- **Low latency**: Tasks are processed immediately by available workers
- **Scalable**: Tested with 100+ concurrent connections

## Troubleshooting

### "Winsock initialization failed"
- Make sure you're running on Windows
- No firewall blocking the port

### "Bind failed"
- Port already in use, try a different port
- Run as administrator

### "Connection refused"
- Server not running
- Check IP address and port

## License

MIT License - Feel free to use and modify!

## Contributing

Contributions welcome! Areas to improve:
- SSL/TLS encryption
- WebSocket support for browser clients  
- Database persistence (SQLite)
- User authentication
- File sharing
