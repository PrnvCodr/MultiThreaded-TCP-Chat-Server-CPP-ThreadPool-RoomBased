@echo off
echo ========================================
echo   Building Chat Server (Windows MSVC)
echo ========================================
echo.

:: Check for Visual Studio
where cl >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo ERROR: Visual Studio compiler (cl.exe) not found.
    echo Please run this from a "Developer Command Prompt for VS"
    echo or run: "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    pause
    exit /b 1
)

echo Compiler found. Building...
echo.

:: Create build directory
if not exist build mkdir build

:: Compile server
echo [1/2] Building server.exe...
cl /nologo /EHsc /std:c++17 /O2 /W3 ^
    /I. ^
    server.cpp sockutil.cpp thread_pool.cpp iocp_server.cpp ^
    connection_manager.cpp chat_room.cpp message_store.cpp ^
    /Fe:build\server.exe ^
    /link ws2_32.lib mswsock.lib

if %ERRORLEVEL% neq 0 (
    echo ERROR: Server build failed!
    pause
    exit /b 1
)

:: Compile client
echo [2/2] Building client.exe...
cl /nologo /EHsc /std:c++17 /O2 /W3 ^
    /I. ^
    client.cpp sockutil.cpp ^
    /Fe:build\client.exe ^
    /link ws2_32.lib

if %ERRORLEVEL% neq 0 (
    echo ERROR: Client build failed!
    pause
    exit /b 1
)

:: Clean up object files
del *.obj 2>nul

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
