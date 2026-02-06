@echo off
echo ========================================
echo   Building Chat Server (MinGW-w64)
echo ========================================
echo.

:: Check for g++
where g++ >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo ERROR: g++ not found.
    echo Please install MinGW-w64 and add it to PATH.
    pause
    exit /b 1
)

echo G++ found. Building...
echo.

:: Create build directory
if not exist build mkdir build

:: Compile server
echo [1/2] Building server.exe...
g++ -std=c++17 -O2 -Wall -D_WIN32_WINNT=0x0601 ^
    -o build/server.exe ^
    server.cpp sockutil.cpp thread_pool.cpp iocp_server.cpp ^
    connection_manager.cpp chat_room.cpp message_store.cpp ^
    -lws2_32 -lmswsock

if %ERRORLEVEL% neq 0 (
    echo ERROR: Server build failed!
    pause
    exit /b 1
)

:: Compile client
echo [2/2] Building client.exe...
g++ -std=c++17 -O2 -Wall -D_WIN32_WINNT=0x0601 ^
    -o build/client.exe ^
    client.cpp sockutil.cpp ^
    -lws2_32

if %ERRORLEVEL% neq 0 (
    echo ERROR: Client build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo   Build successful!
echo ========================================
echo.
echo Executables created in build\ directory:
echo   - server.exe
echo   - client.exe
echo.
echo To run:
echo   1. Open terminal: build\server.exe 8080
echo   2. Open another terminal: build\client.exe 127.0.0.1 8080
echo.
pause
