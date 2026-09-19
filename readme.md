Multiplayer Coin Collector Game

Overview
A real-time multiplayer game built using C++ and SFML. 


Prerequisites (Linux / WSL)

You need the SFML development libraries installed:
```bash
sudo apt-get update
sudo apt-get install libsfml-dev
```

Required: Font Setup

The game relies strictly on arial.ttf for rendering scores. You must copy this file to the project folder for the score to appear.

WSL Users:
Copy it directly from your Windows partition:
```bash
cp /mnt/c/Windows/Fonts/arial.ttf .
```

macOS Users:
copy it from your system library (renaming to lowercase to match the code):
```bash
cp /Library/Fonts/Arial.ttf ./arial.ttf
```

Native Linux Users:
Arial is not open-source. You must install the Microsoft fonts package first:
```bash
sudo apt-get install ttf-mscorefonts-installer
cp /usr/share/fonts/truetype/msttcorefonts/Arial.ttf ./arial.ttf
```

Compilation Instructions

The project uses g++ with C++17 standard.

1. Compile the Server
```bash
g++ -std=c++17 server.cpp -o server -lsfml-network -lsfml-system
```

2. Compile the Client
```bash
g++ -std=c++17 client.cpp -o client -lsfml-graphics -lsfml-window -lsfml-network -lsfml-system
```

How to Run

Start the Server (Host):
```bash
./server
```

Start Client 1 (Open new terminal):
```bash
./client
```

Start Client 2 (Open new terminal):
```bash
./client
```

Thank you.