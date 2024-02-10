#include "SimpleWebSocket.h"
#include <thread>



std::thread SimpleWebSocket::startWebSocketServer() {
    std::thread t1(&SimpleWebSocket::_startWebSocketServer, this);
   // this->_startWebSocketServer();
    return t1;
}

int SimpleWebSocket::_startWebSocketServer() {

    int iResult;

    this->ListenSocket = INVALID_SOCKET;
    this->ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    int iSendResult;


    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &this->wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    this->ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (this->ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(this->ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(this->ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(this->ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(this->ListenSocket);
        WSACleanup();
        return 1;
    }


    std::cout << "WebSocket started. Waiting for connection..." << std::endl;

    //Loop to continue accept clients socket
    while (true)
    {

       
        // Accept a client socket
        this->ClientSocket = accept(this->ListenSocket, NULL, NULL);
        if (this->ClientSocket == INVALID_SOCKET) {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(this->ListenSocket);
            WSACleanup();
            return 1;
        }

        // No longer need server socket
        //closesocket(this->ListenSocket);

        

        //Start response success handshake result
        iResult = recv(this->ClientSocket, this->recvbuf, this->recvbuflen, 0);
        if (iResult == 0)
        {
            printf("Connection closing...\n");
            break;
        }
        else if (iResult < 0)
        {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(this->ClientSocket);
            WSACleanup();
            return 1;
        }

        this->recvbuf[iResult] = 0;

        char sendbuf[DEFAULT_BUFLEN];
        size_t sendbuf_size = 0;

        // see if it's requesting a key
        char* pKey = strstr(this->recvbuf, "Sec-WebSocket-Key:");
        if (pKey)
        {
            // parse just the key part
            pKey = strchr(pKey, ' ') + 1;
            char* pEnd = strchr(pKey, '\r');
            *pEnd = 0;

            char key[256];
            _snprintf_s(key, _countof(key), "%s%s", pKey, WEBSOCKET_KEY);

            unsigned char result[20];
            const unsigned char* pSha1Key = sha1(key);

            // endian swap each of the 5 ints
            for (int i = 0; i < 5; i++)
            {
                for (int c = 0; c < 4; c++)
                    result[i * 4 + c] = pSha1Key[i * 4 + (4 - c - 1)];
            }

            pKey = base64_encode(result, 20);

            const char* pTemplateResponse = "HTTP/1.1 101 Switching Protocols\r\n"
                "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                "Sec-WebSocket-Accept: %s\r\n\r\n";

            _snprintf_s(sendbuf, _countof(sendbuf), pTemplateResponse, pKey);
            sendbuf_size = strlen(sendbuf);
            iSendResult = send(ClientSocket, sendbuf, (int)sendbuf_size, 0);

            if (iSendResult == SOCKET_ERROR)
            {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(this->ClientSocket);
                WSACleanup();
                return 1;
            }

            //End send handhsae success result
        }


        //Manage list clients
        this->socketCliensts.push_back(this->ClientSocket);

        //Spawn e thred to handle this ClientSocket's message below
        std::thread t1 = std::thread(&SimpleWebSocket::_handleClientMessage, this, this->ClientSocket);
       // this->clientThreads.push_back(t1);
        t1.detach();
        //Continue this loop to receive another Socket Clients and also to keep this thread active
    }

    return 0;
}




int SimpleWebSocket::sendSocketMessageString(SOCKET _client,std::string message) {
    if (_client == INVALID_SOCKET)
        return 0;

    char sendbuf[DEFAULT_BUFLEN];
    size_t sendbuf_size = 0;

   // char _recvbuf[DEFAULT_BUFLEN];
   // strcpy_s(_recvbuf, sizeof(_recvbuf), _xrecvbuf);


    // else read, print the response, and echo it back to the server

    _websocket_header* h = (_websocket_header*)this->recvbuf;



    char client_msg[DEFAULT_BUFLEN];
    strcpy_s(client_msg, DEFAULT_BUFLEN, message.c_str());

    char* pData;

    h = (_websocket_header*)sendbuf;
    *h = _websocket_header{};

    h->opcode = 0x1; //0x1 = text, 0x2 = blob
    h->fin = 1;

    char text[DEFAULT_BUFLEN];
    _snprintf_s(text, sizeof(text), "%s", client_msg);
    std::cout << "xxx" << std::endl;
    unsigned long long msg_length = strlen(text);

    sendbuf_size = sizeof(_websocket_header);
   
    if (msg_length <= 125)
    {
        pData = sendbuf + sizeof(_websocket_header);
        h->len = msg_length;
    }
    else if (msg_length <= 0xffff)
    {
        h->len = 126;

        _extended_16* extended = (_extended_16*)(sendbuf + sendbuf_size);
        sendbuf_size += sizeof(_extended_16);

        extended->value[0] = (msg_length >> 8) & 0xff;
        extended->value[1] = msg_length & 0xff;
    }
    else
    {
        h->len = 127;

        _extended_64* extended = (_extended_64*)(sendbuf + sendbuf_size);
        sendbuf_size += sizeof(_extended_64);

        extended->value[0] = ((msg_length >> 56) & 0xff);
        extended->value[1] = ((msg_length >> 48) & 0xff);
        extended->value[2] = ((msg_length >> 40) & 0xff);
        extended->value[3] = ((msg_length >> 32) & 0xff);
        extended->value[4] = ((msg_length >> 24) & 0xff);
        extended->value[5] = ((msg_length >> 16) & 0xff);
        extended->value[6] = ((msg_length >> 8) & 0xff);
        extended->value[7] = ((msg_length >> 0) & 0xff);
    }

    pData = sendbuf + sendbuf_size;

    memcpy(pData, text, (size_t)msg_length);
    sendbuf_size += (size_t)msg_length;
    

    int iSendResult = send(_client, sendbuf, (int)sendbuf_size, 0);
    std::cout << "Send result = " << iSendResult << std::endl;

    if (iSendResult == SOCKET_ERROR)
    {
        std::cout << "Send Failes " << iSendResult << std::endl;
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(_client);
        WSACleanup();
        return 1;
    }
    std::cout << "Retune " << iSendResult << std::endl;
    return 1;
}




int SimpleWebSocket::_handleClientMessage(SOCKET _client) {

    while (true)
    {
        char recvbuf[DEFAULT_BUFLEN];
        int iResult = recv(_client, recvbuf, sizeof recvbuf, 0);

        if (iResult == 0)
        {
            printf("Connection closing...\n");
            break;
        }
        else if (iResult < 0)
        {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(_client);
            WSACleanup();
            return 1;
        }

        recvbuf[iResult] = 0;
        // else read, print the response, and echo it back to the server
        _websocket_header* h = (_websocket_header*)recvbuf;

        _mask_key* mask_key;

        unsigned long long length;

        if (h->len < 126)
        {
            length = h->len;
            mask_key = (_mask_key*)(recvbuf + sizeof(_websocket_header));
        }
        else if (h->len == 126)
        {
            _extended_16* extended = (_extended_16*)(recvbuf + sizeof(_websocket_header));

            length = (extended->value[0] << 8) | extended->value[1];
            mask_key = (_mask_key*)(recvbuf + sizeof(_websocket_header) + sizeof(_extended_16));
        }
        else
        {
            _extended_64* extended = (_extended_64*)(recvbuf + sizeof(_websocket_header));

            length = (((unsigned long long) extended->value[0]) << 56) | (((unsigned long long) extended->value[1]) << 48) | (((unsigned long long) extended->value[2]) << 40) |
                (((unsigned long long) extended->value[3]) << 32) | (((unsigned long long) extended->value[4]) << 24) | (((unsigned long long) extended->value[5]) << 16) |
                (((unsigned long long) extended->value[6]) << 8) | (((unsigned long long) extended->value[7]) << 0);

            mask_key = (_mask_key*)(recvbuf + sizeof(_websocket_header) + sizeof(_extended_64));
        }

        char* client_msg = ((char*)mask_key) + sizeof(_mask_key);

        if (h->mask)
        {
            for (int i = 0; i < length; i++)
                client_msg[i] = client_msg[i] ^ mask_key->value[i % 4];
        }

        //printf("Client: %s\r\n", client_msg);


        //std::cout << "Some one said " << client_msg << std::endl;
        if(this->handleClientMessage != NULL)
            this->handleClientMessage(_client, client_msg);
    }

}
