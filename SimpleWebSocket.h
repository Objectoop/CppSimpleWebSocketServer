#pragma once
#ifndef SIMPLEWEBSOCKET_H_
#define SIMPLEWEBSOCKET_H_
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include "sha1.h"
#include <vector>


#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN  4096
#define DEFAULT_PORT    "4221"
#define WEBSOCKET_KEY   "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

// WHAT IT IS:
// Server created to listen for a single connection
// A websocket is then established and the server echos back the client's text

// WHAT IT IS NOT:
// Robust or meant for anything other than a basic understanding of a C/C++ websocket implementation



// websocket RFC:
// https://tools.ietf.org/html/rfc6455#section-5.2

// basic winsock server code taken from:
// https://docs.microsoft.com/en-us/windows/desktop/WinSock/complete-server-code


// Header comes in most significant bit as left most
// In practice this means each group is endian swapped
// So I've modified this header accordingly
//#pragma pack (push, 1)
struct _websocket_header
{
    unsigned char opcode : 4;

    unsigned char rsv3 : 1;
    unsigned char rsv2 : 1;
    unsigned char rsv1 : 1;
    unsigned char fin : 1;

    unsigned char len : 7;
    unsigned char mask : 1;
};

struct _extended_16
{
    unsigned char value[2];
};

struct _extended_64
{
    unsigned char value[8];
};

struct _mask_key
{
    unsigned char value[4];
};


//#pragma pack (pop)
class SimpleWebSocket
{
    public:
        WSADATA wsaData;
        SOCKET ListenSocket = INVALID_SOCKET;
        SOCKET ClientSocket = INVALID_SOCKET;
        char recvbuf[DEFAULT_BUFLEN];
        int recvbuflen = DEFAULT_BUFLEN;
        std::thread startWebSocketServer();
        int _handleClientMessage(SOCKET _client);
        int sendSocketMessageString(SOCKET _client,std::string message);
        int _startWebSocketServer();


        void (*handleClientMessage)(SOCKET, std::string);

        std::vector<SOCKET> socketCliensts;
        std::vector<std::thread> clientThreads;
  

    private:
    


};


#endif

