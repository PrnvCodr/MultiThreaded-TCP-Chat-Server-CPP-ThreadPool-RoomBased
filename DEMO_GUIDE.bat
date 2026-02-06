@echo off
echo ============================================
echo   Multi-Threaded Chat Server - Demo Guide
echo ============================================
echo.
echo This guide will help you test all features of the chat server.
echo.
echo ============================================
echo   STEP 1: START THE SERVER
echo ============================================
echo.
echo Open Terminal 1 and run:
echo   cd build
echo   server.exe 8080
echo.
echo You should see:
echo   - "High-Performance Chat Server v2.0"
echo   - "Server listening on port 8080"
echo.
pause
echo.
echo ============================================
echo   STEP 2: CONNECT CLIENT #1
echo ============================================
echo.
echo Open Terminal 2 (new terminal) and run:
echo   cd build
echo   client.exe 127.0.0.1 8080
echo.
echo When connected, type your username (e.g., "Alice")
echo.
pause
echo.
echo ============================================
echo   STEP 3: CONNECT CLIENT #2
echo ============================================
echo.
echo Open Terminal 3 (new terminal) and run:
echo   cd build
echo   client.exe 127.0.0.1 8080
echo.
echo When connected, type your username (e.g., "Bob")
echo.
pause
echo.
echo ============================================
echo   STEP 4: TEST BASIC CHAT
echo ============================================
echo.
echo In Alice's terminal, type: Hello everyone!
echo In Bob's terminal, type: Hi Alice!
echo.
echo ^> Both should see each other's messages
echo.
pause
echo.
echo ============================================
echo   STEP 5: TEST COMMANDS
echo ============================================
echo.
echo Try these commands in any client:
echo.
echo   #help       - Show all available commands
echo   #rooms      - List all chat rooms
echo   #online     - See who's online
echo   #history 5  - View last 5 messages
echo.
pause
echo.
echo ============================================
echo   STEP 6: TEST CHAT ROOMS
echo ============================================
echo.
echo In Alice's terminal:
echo   #create gaming
echo   (Creates a new room called "gaming")
echo.
echo In Bob's terminal:
echo   #join gaming
echo   (Joins the gaming room)
echo.
echo Now Alice and Bob are in #gaming room!
echo Try chatting - only people in the same room see messages.
echo.
pause
echo.
echo ============================================
echo   STEP 7: TEST PRIVATE MESSAGING
echo ============================================
echo.
echo In Alice's terminal:
echo   #whisper Bob This is a secret message!
echo.
echo ^> Only Bob will see this message
echo.
pause
echo.
echo ============================================
echo   STEP 8: TEST ADMIN COMMANDS
echo ============================================
echo.
echo   #kick Bob   - Kicks Bob from server
echo   #mute Bob 30 - Mutes Bob for 30 seconds
echo   #ban Bob    - Bans Bob's IP address
echo.
echo (Use these carefully - they really work!)
echo.
pause
echo.
echo ============================================
echo   STEP 9: DISCONNECT
echo ============================================
echo.
echo Type: #exit
echo Or press Ctrl+C in client terminal
echo.
echo To stop the server, press Ctrl+C in server terminal
echo.
echo ============================================
echo   DEMO COMPLETE!
echo ============================================
echo.
echo All features tested:
echo   [x] Multi-client connections
echo   [x] Real-time messaging
echo   [x] Chat rooms (create/join/leave)
echo   [x] Private whisper messages
echo   [x] Message history
echo   [x] Admin commands (kick/mute/ban)
echo   [x] User list
echo.
pause
