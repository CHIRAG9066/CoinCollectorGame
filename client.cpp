#include "shared.hpp"
#include <SFML/Graphics.hpp>
#include <deque>
#include <iostream>
#include <iomanip> 
#include <algorithm> 

struct State {
    sf::Time timestamp; 
    struct P {int id; float x,y; int score; };
    struct C {float x, y;};
    std::vector<P> players;
    std::vector<C> coins;
};

int main() {
    // anti-aliasing for smoother shapes ;)
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    
    sf::RenderWindow window(sf::VideoMode(800, 600), "Krafton assignment - Client", sf::Style::Default, settings);
    window.setFramerateLimit(60);

    sf::TcpSocket socket;
    // connection attempt
    sf::Socket::Status status = socket.connect(SERVER_IP, PORT);
    if (status != sf::Socket::Done) {
        std::cerr << "Error: Could not connect to " << SERVER_IP << ":" << PORT << "\n";
        return -1;
    }
    socket.setBlocking(false);

    int myID = -1;
    std::deque<State> stateBuffer; 
    
    // render time = current time - (latency + jitter Buffer)
    // using total 300ms (200ms Simulated Latency + 100ms Buffer)
    sf::Time renderDelay = sf::milliseconds(300);
    sf::Clock localTime; 
    
    sf::Font font;
    bool hasFont = font.loadFromFile("arial.ttf"); //font for the score rendering

    while(window.isOpen()){
        sf::Event e;
        while(window.pollEvent(e)) {
            if (e.type==sf::Event::Closed) window.close();
        }

        // RECEIVE DATA
        sf::Packet pkt;
        while (socket.receive(pkt) == sf::Socket::Done) {
            int type;
            if (pkt>>type) {
                if (type==JOIN_ACK) {
                    pkt>>myID;
                    std::cout << "[Network] Connected. Assigned ID: " << myID<<std::endl;
                }
                else if(type ==UPDATE){
                    State s;
                    s.timestamp = localTime.getElapsedTime(); // T_arrival

                    int pCount, cCount;
                    if (pkt >> pCount) {
                        for(int i=0; i<pCount; i++) {
                            State::P p; 
                            pkt >> p.id >> p.x >> p.y >> p.score;
                            s.players.push_back(p); }
                    }
                    
                    if (pkt >> cCount) {
                        for(int i=0; i<cCount; i++) {
                            State::C c; 
                            pkt >>c.x>> c.y;
                            s.coins.push_back(c);
                        }}
                    
                    stateBuffer.push_back(s);
                    // Memory Management: remove states older than 2 seconds
                    while (stateBuffer.size() > 0 && 
                           (localTime.getElapsedTime() - stateBuffer.front().timestamp).asSeconds()>2.0f) {
                        stateBuffer.pop_front();
                    }
                }
            }
        }

        // --- SEND INPUT ---
        if (myID != -1 && window.hasFocus()) {
            float dx=0, dy=0;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) dy = -1;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) dy = 1;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) dx = -1;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) dx = 1;
            
            if (dx!=0 || dy!=0) {
                sf::Packet inputPkt;
                inputPkt << (int)INPUT << dx<<dy;
                socket.send(inputPkt);
            }
        }

        // RENDER--
        window.clear( sf::Color(30, 30,40)); 
        
        sf::Time renderTime = localTime.getElapsedTime() -renderDelay;
        State* s1 = nullptr; State* s2 = nullptr;

        // Underflow check
        if (stateBuffer.size() >= 2) {
            for (size_t i=0; i<stateBuffer.size() - 1; i++) {
                if (stateBuffer[i].timestamp <= renderTime && 
                    stateBuffer[i+1].timestamp >= renderTime) {
                    s1 = &stateBuffer[i];
                    s2 = &stateBuffer[i+1];
                    break;
                }
            }
        }

        auto drawPlayer = [&](float x, float y, int id, int score) {
            // Clamping Logic
            const float RADIUS = 20.0f;
            x = std::clamp(x, RADIUS, 800.0f - RADIUS);
            y = std::clamp(y, RADIUS, 600.0f - RADIUS);

            sf::CircleShape shape(RADIUS);
            shape.setOrigin(RADIUS, RADIUS);
            shape.setPosition(x,y);
            
            if (id ==myID) {
               shape.setFillColor(sf::Color(100,255,100)); // Green for player
                shape.setOutlineThickness(3);
                shape.setOutlineColor(sf::Color::White);
            }else{
                shape.setFillColor(sf::Color(255,100,100)); // Red for enemies
            }
            window.draw(shape);

            // SCORING TEXT
            if (hasFont) {
                sf::Text text;
                text.setFont(font);
                text.setString(std::to_string(score));
                text.setCharacterSize(16);
                text.setFillColor(sf::Color::White);
                text.setOutlineColor(sf::Color::Black);
                text.setOutlineThickness(1);
                
                sf::FloatRect bounds =text.getLocalBounds();
                text.setOrigin(bounds.width/2, bounds.height/2);
                text.setPosition(x, y-35); // score above the circle
                window.draw(text);
            }
        };

        if (s1&&s2) {
            // INTERPOLATION 
            float t1 = s1->timestamp.asSeconds();
            float t2 = s2->timestamp.asSeconds();
            float tr = renderTime.asSeconds();
            float alpha = (tr-t1)/(t2-t1);

            // Draw Coins
            for (auto& c : s2->coins) {
                sf::CircleShape coin(8);
                coin.setOrigin(8, 8);
                coin.setFillColor(sf::Color(255, 215,0));
                 coin.setOutlineThickness(2);
                coin.setOutlineColor(sf::Color(255,140, 0));
                coin.setPosition(c.x, c.y);
                window.draw(coin);
            }

            // Draw Players
            for (auto& p1 : s1->players) {
                for (auto& p2 : s2->players) {
                    if (p1.id == p2.id) {
                        float x = p1.x + (p2.x-p1.x)*alpha;
                        float y = p1.y + (p2.y-p1.y)*alpha;
                        drawPlayer(x, y, p1.id, p1.score); // Pass score here
                        break;
                    }
                }}
        } 
        else if (!stateBuffer.empty()){
            // fallback mode
            State& latest = stateBuffer.back();
            for (auto& c : latest.coins) {
                sf::CircleShape coin(8);
                coin.setOrigin(8,8);
                coin.setFillColor(sf::Color(255, 215, 0));
                coin.setPosition(c.x, c.y);
                window.draw(coin);
            }
            for (auto& p : latest.players) {
                drawPlayer(p.x, p.y, p.id, p.score);
            }
        }
        else {
            // waiting o
             sf::RectangleShape box(sf::Vector2f(20, 20));
             box.setFillColor(sf::Color::White);
             box.setPosition(400, 300);
             box.setRotation(localTime.getElapsedTime().asMilliseconds() / 2.0f);
             window.draw(box);
             
             if(hasFont) {
                 sf::Text t; t.setFont(font);
                 t.setString("connecting...");
                 t.setPosition(360, 340);
                 t.setCharacterSize(14);
                 window.draw(t) ;  }
        }

        window.display();
    }
    return 0;
}