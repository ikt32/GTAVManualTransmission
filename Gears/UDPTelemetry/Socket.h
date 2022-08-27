#pragma once

#include "../Util/Logger.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

class Socket {
public:
    Socket() = default;

    void Start(const std::string& address, u_short destPort) {
        const char* addressStr = address.c_str();

        logger.Write(INFO, "[Telemetry] Starting UDP on %s:%d", addressStr, destPort);
        mStarted = false;
        WSAData data{};
        int result = WSAStartup(MAKEWORD(2, 2), &data);

        if (result != 0) {
            logger.Write(ERROR, "[Telemetry] WSAStartup failed with %d", result);
            return;
        }

        mLocal.sin_family = AF_INET;
        result = inet_pton(AF_INET, addressStr, &mLocal.sin_addr);
        if (result != 1) {
            logger.Write(ERROR, "[Telemetry] inet_pton result was [%d]", result);
            logger.Write(ERROR, "[Telemetry] inet_pton error was [%d]", WSAGetLastError());
            return;
        }

        mLocal.sin_port = 0; // choose any

        mDest.sin_family = AF_INET;
        inet_pton(AF_INET, addressStr, &mDest.sin_addr);
        if (result != 1) {
            logger.Write(ERROR, "[Telemetry] inet_pton result was [%d]", result);
            logger.Write(ERROR, "[Telemetry] inet_pton error was [%d]", WSAGetLastError());
            return;
        }
        mDest.sin_port = htons(destPort);
        
        // create the socket
        mSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        // bind to the local address
        result = bind(mSocket, reinterpret_cast<sockaddr*>(&mLocal), sizeof(mLocal));

        if (result == SOCKET_ERROR) {
            logger.Write(ERROR, "[Telemetry] bind failed with %d", WSAGetLastError());
            return;
        }

        mStarted = true;
    }

    int SendPacket(char* packet, int size) {
        return sendto(mSocket, packet, size, 0, reinterpret_cast<sockaddr*>(&mDest), sizeof(mDest));
    }

    bool Started() const {
        return mStarted;
    }

private:
    sockaddr_in mDest{};
    sockaddr_in mLocal{};
    SOCKET mSocket{};
    bool mStarted = false;
};
