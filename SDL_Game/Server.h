#pragma once
#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "25566"

SOCKET setupServer(std::string address, int port) {
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
    }

    hostent* localHost;
    char* localIP;
    struct sockaddr_in saServer;

    // Resolve the server address and port
    localHost = gethostbyname(address.c_str());
    localIP = inet_ntoa(*(struct in_addr*) * localHost->h_addr_list);

    // Set up the sockaddr structure
    saServer.sin_family = AF_INET;
    saServer.sin_addr.s_addr = inet_addr(localIP);
    saServer.sin_port = htons(port);

    // Create a SOCKET for connecting to server
    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, (SOCKADDR*)&saServer, sizeof(saServer));
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
    }

    printf("Server binded on %s:%i\n", address.c_str(), port);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
    }
    printf("Client connected\n");
    // No longer need server socket
    closesocket(ListenSocket);

    return ClientSocket;
}
int closeServer(SOCKET ClientSocket) {
    int iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}