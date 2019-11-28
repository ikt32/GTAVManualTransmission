#include <winsock2.h>

class Socket {
public:
    Socket() = default;

    void Start(u_short destPort) {
        WSAData data;
        WSAStartup(MAKEWORD(2, 2), &data);

        local.sin_family = AF_INET;
        local.sin_addr.s_addr = inet_addr("127.0.0.1");
        local.sin_port = 0; // choose any

        dest.sin_family = AF_INET;
        dest.sin_addr.s_addr = inet_addr("127.0.0.1");
        dest.sin_port = htons(destPort);
        
        // create the socket
        s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        // bind to the local address
        bind(s, (sockaddr*)&local, sizeof(local));
    }

    int SendPacket(char* packet, size_t size) {
        // send the pkt
        int ret = sendto(s, packet, size, 0, (sockaddr*)&dest, sizeof(dest));
        return ret;
    }
private:
    sockaddr_in dest;
    sockaddr_in local;
    SOCKET s;
};
