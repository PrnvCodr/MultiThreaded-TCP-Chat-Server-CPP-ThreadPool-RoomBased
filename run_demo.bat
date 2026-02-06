@echo off
set "BUILD_DIR=build"
set "SERVER_EXE=%BUILD_DIR%\server.exe"
set "CLIENT_EXE=%BUILD_DIR%\client.exe"

if not exist "%SERVER_EXE%" (
    echo Error: Server not found. Run build_mingw.bat first.
    pause
    exit /b
)

echo =======================================================
echo    Chat Server Demo Launcher 
echo =======================================================
echo.
echo Launching Server...
start "Chat Server" cmd /k "%SERVER_EXE% 8080"
timeout /t 2 >nul

echo Launching Client 1 (Alice)...
start "Client: Alice" cmd /k "%CLIENT_EXE% 127.0.0.1 8080"
timeout /t 1 >nul

echo Launching Client 2 (Bob)...
start "Client: Bob" cmd /k "%CLIENT_EXE% 127.0.0.1 8080"
timeout /t 1 >nul

echo Launching Client 3 (Admin)...
start "Client: Admin" cmd /k "%CLIENT_EXE% 127.0.0.1 8080"
timeout /t 1 >nul

cls
echo =======================================================
echo             DEMO WALKTHROUGH SCRIPT
echo =======================================================
echo.
echo 1. SETUP USERS:
echo    - In window "Alice": Type "Alice"
echo    - In window "Bob":   Type "Bob"
echo    - In window "Admin": Type "Admin"
echo.
echo 2. SHOW ROOMS:
echo    - Alice: Type "#create #linkedin"
echo    - Bob:   Type "#join #linkedin"
echo    - Bob:   Type "Hi Alice, checking out your new server!"
echo.
echo 3. SHOW PRIVATE MESSAGIN:
echo    - Alice: Type "#whisper Bob secret message here"
echo.
echo 4. SHOW ADMIN POWERS:
echo    - Admin: Type "#online"
echo    - Admin: Type "#mute Bob 10"  (Mutes Bob for 10s)
echo    - Bob:   Type "Can I speak?"  (Should fail)
echo.
echo 5. CLEANUP:
echo    - Admin: Type "#kick Bob"
echo.
echo =======================================================
echo    Minimize this window and start recording!
echo =======================================================
pause
