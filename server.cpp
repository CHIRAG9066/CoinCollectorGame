#include "shared.hpp"
#include<iostream>
#include <vector>
#include <cmath>
#include <list>

struct Player {
    sf::TcpSocket* socket;
    int id;
    float x, y;
    int score;
};

struct Coin {
    float x,y;
};

struct pending_packet {
    sf::Packet data;
    sf::TcpSocket* recipient; 
    sf::Clock sendTimer;      // Tracks how long it's been delayed
};

int main() {
    std::cout << "--- SERVER STARTED (Latency: " << LATENCY_MS << "ms) ---\n";

    sf::TcpListener listener;
    listener.listen(PORT);
    sf::SocketSelector selector;
    selector.add(listener);

    std::list<Player>players;
    std::vector<Coin>coins;
    std::vector<pending_packet>networkQueue; // Simulates the lag

    // Spawn some random coins initially
    for(int i=0; i<5; i++){
        coins.push_back({(float)(rand()%700 + 50), (float)(rand()%500 +50)});
    } 

    sf::Clock gameClock;
    int idCounter = 1;

    while (true) {
        // --- NETWORK IO ---
        if (selector.wait(sf::milliseconds(5))) {
            // New Connection?
            if (selector.isReady(listener)) {
                auto* sock = new sf::TcpSocket();
                if (listener.accept(*sock) == sf::Socket::Done) {
                    int newID = idCounter++;
                    players.push_back({sock, newID, 400.0f, 300.0f,0});
                    selector.add(*sock);
                    std::cout << "Player " << newID <<" connected.\n";

                    // Send ack
                    sf::Packet ack;
                    ack << (int)JOIN_ACK << newID;
                    sock->send(ack);
                }}

            // Data from Players
            for (auto& p : players) {
                if (selector.isReady(*p.socket)) {
                    sf::Packet pkt;
                    if (p.socket->receive(pkt) == sf::Socket::Done) {
                        int type; pkt >> type;
                    if (type == INPUT) {
                        float dx, dy;
                        pkt >>dx>>dy;
                        // update position
                        p.x += dx*SPEED*TICK_RATE;  p.y += dy*SPEED*TICK_RATE;
                        } }
                }
            }}

        // --- GAME LOGIC ---
        // collision logic
        for (auto& p : players) {
            for (auto it = coins.begin(); it != coins.end(); ) {
                float dist = std::sqrt(std::pow(p.x - it->x, 2) + std::pow(p.y - it->y, 2));
                if (dist<30.0f) { // player radium -> 20 + coin radius -> 10
                    p.score++;
                    it = coins.erase(it); 
                    coins.push_back({(float)(rand()%700+50), (float)(rand()%500+50)});
                } else ++it;
            }
        }

        // --- SEND UPDATES (With 200ms Delay) ---
        if (gameClock.getElapsedTime().asSeconds() > TICK_RATE) {
            gameClock.restart();

            sf::Packet updatePkt;
            updatePkt <<(int)UPDATE;
            
            // Pack players
            updatePkt << (int)players.size();
            for (auto&p : players) updatePkt << p.id << p.x << p.y << p.score;
            
            // Pack coins
            updatePkt << (int)coins.size();
            for (auto&c : coins) updatePkt << c.x << c.y;

            //  add to queue for lag, not seding it immediately
            pending_packet pending;
            pending.data = updatePkt;
            pending.recipient = nullptr; // Broadcast
            pending.sendTimer.restart(); // Start waiting now
            networkQueue.push_back(pending);
        }

        // --- PROCESS QUEUE ---
        // Check if packets have waited for 200ms(latency)
        for (auto it = networkQueue.begin(); it!=networkQueue.end();){
            if (it->sendTimer.getElapsedTime().asMilliseconds()>= LATENCY_MS) {
                // time to actually send
                if (it->recipient==nullptr) {
                    for (auto& p : players) p.socket->send(it->data);
                }
                it = networkQueue.erase(it);
            }else ++it;
        }
    }
}