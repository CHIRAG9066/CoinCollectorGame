#pragma once
#include <SFML/Network.hpp>

//Ports and Addresses
const int PORT = 5000;
const std::string SERVER_IP = "127.0.0.1";

//Game Constants
const float SPEED = 300.0f;
const float TICK_RATE = 1.0f / 30.0f; // 30 Hz updates
const int LATENCY_MS = 200;           // Required LAG

//Packet Headers
enum PacketType {
    JOIN_REQ = 0,
    JOIN_ACK = 1,
    INPUT    = 2,
    UPDATE   = 3
};