#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "Queue.h"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"
#define DEFAULT_PORT2 "27017"
#define MAX_CLIENT_NUM 50
bool InitializeWindowsSockets();

typedef struct params_st
{
    ORDER_NODE** glava;
    CRITICAL_SECTION* cs;
}PARAMS;

DWORD WINAPI statusDequeueThread(LPVOID lpParam);
SOCKET acceptedSocket2[MAX_CLIENT_NUM];

int  main(void)
{
    // Socket used for listening for new clients 
    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET listenSocket2 = INVALID_SOCKET;
    // Socket used for communication with client
    SOCKET acceptedSocket[MAX_CLIENT_NUM];
    
    // variable used to store function return value
    int iResult;
    // Buffer used for storing incoming data
    char recvbuf[DEFAULT_BUFLEN];

    if (InitializeWindowsSockets() == false)
    {
        // we won't log anything since it will be logged
        // by InitializeWindowsSockets() function
        return 1;
    }

    // Prepare address information structures
    addrinfo* resultingAddress = NULL;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4 address
    hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
    hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
    hints.ai_flags = AI_PASSIVE;     // 

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &resultingAddress);
    if (iResult != 0)
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    addrinfo* resultingAddress2 = NULL;
    addrinfo hints2;

    memset(&hints2, 0, sizeof(hints2));
    hints2.ai_family = AF_INET;       // IPv4 address
    hints2.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
    hints2.ai_protocol = IPPROTO_TCP; // Use TCP protocol
    hints2.ai_flags = AI_PASSIVE;     // 

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT2, &hints, &resultingAddress2);
    if (iResult != 0)
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }
    // Create a SOCKET for connecting to server
    listenSocket = socket(AF_INET,      // IPv4 address famly
        SOCK_STREAM,  // stream socket
        IPPROTO_TCP); // TCP

    if (listenSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        WSACleanup();
        return 1;
    }

    listenSocket2 = socket(AF_INET,      // IPv4 address famly
        SOCK_STREAM,  // stream socket
        IPPROTO_TCP); // TCP

    if (listenSocket2 == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(resultingAddress2);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket - bind port number and local address 
    // to socket
    iResult = bind(listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    iResult = bind(listenSocket2, resultingAddress2->ai_addr, (int)resultingAddress2->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(resultingAddress2);
        closesocket(listenSocket2);
        WSACleanup();
        return 1;
    }
    // Since we don't need resultingAddress any more, free it
    freeaddrinfo(resultingAddress);
    freeaddrinfo(resultingAddress2);

    // Set listenSocket in listening mode
    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    iResult = listen(listenSocket2, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    printf("Server initialized, waiting for clients.\n");
    int clientNum = 0;
    int clientNum2 = 0;

    unsigned long mode = 1; //non-blocking mode
    iResult = ioctlsocket(listenSocket, FIONBIO, &mode);
    if (iResult != NO_ERROR)
        printf("ioctlsocket failed with error: %ld\n", iResult);

    //inicijalizacija threadova i redova
    ORDER_NODE* ordersQueue;
    CRITICAL_SECTION ordersCS;
    InitializeCriticalSection(&ordersCS);
    Init(&ordersQueue, ordersCS);

    DWORD print1ID;
    HANDLE handle1;
    PARAMS params1;
    params1.glava = &ordersQueue;
    params1.cs = &ordersCS;
    handle1 = CreateThread(NULL, 0, &statusDequeueThread, &params1, 0, &print1ID);

    do
    {
        //primanje konekcije
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listenSocket, &readfds);
        FD_SET(listenSocket2, &readfds);
        timeval timeVal;
        timeVal.tv_sec = 1;
        timeVal.tv_usec = 0;

        int result = select(0, &readfds, NULL, NULL, &timeVal);

        if (result == 0)
        {
            // vreme za cekanje je isteklo
        }
        else if (result == SOCKET_ERROR)
        {
            //desila se greska prilikom poziva funkcije
        }
        else
        {
            if (FD_ISSET(listenSocket, &readfds))
            {
                acceptedSocket[clientNum] = accept(listenSocket, NULL, NULL);

                if (acceptedSocket[clientNum] == INVALID_SOCKET)
                {
                    printf("accept failed with error: %d\n", WSAGetLastError());
                    closesocket(listenSocket);
                    WSACleanup();
                    return 1;
                }

                mode = 1;
                iResult = ioctlsocket(acceptedSocket[clientNum], FIONBIO, &mode);
                if (iResult != NO_ERROR)
                    printf("ioctlsocket failed with error: %ld\n", iResult);

                clientNum++;
            }
            if (FD_ISSET(listenSocket2, &readfds))
            {
                acceptedSocket2[clientNum2] = accept(listenSocket2, NULL, NULL);

                if (acceptedSocket2[clientNum2] == INVALID_SOCKET)
                {
                    printf("accept failed with error: %d\n", WSAGetLastError());
                    closesocket(listenSocket);
                    WSACleanup();
                    return 1;
                }

                mode = 1;
                iResult = ioctlsocket(acceptedSocket2[clientNum2], FIONBIO, &mode);
                if (iResult != NO_ERROR)
                    printf("ioctlsocket failed with error: %ld\n", iResult);

                clientNum2++;
            }
        }

        //primanje zahteva za taxi
        fd_set readfdsClients;
        FD_ZERO(&readfdsClients);
        for (int i = 0; i < clientNum; i++)
        {
            FD_SET(acceptedSocket[i], &readfdsClients);
        }

        result = select(0, &readfdsClients, NULL, NULL, &timeVal);

        if (result == 0)
        {
            // vreme za cekanje je isteklo
        }
        else if (result == SOCKET_ERROR)
        {
            //desila se greska prilikom poziva funkcije
        }
        else
        {
            for (int i = 0; i < clientNum; i++)
            {
                if (FD_ISSET(acceptedSocket[i], &readfdsClients))
                {
                    iResult = recv(acceptedSocket[i], recvbuf, DEFAULT_BUFLEN, 0);
                    if (iResult > 0)
                    {
                        //kada smo primili poruku smestimo u queue u zavisnosti od topica
                        CLIENT_MESSAGE* v = (CLIENT_MESSAGE*)recvbuf;
                        
                        //prosledi taxisti
                        // ? ? ?
                        //TODO: Da li raditi neki algoritam ili ici jednostavno round-robin
                        Enqueue(&ordersQueue, *v, ordersCS);
                    }
                    else if (iResult == 0)
                    {
                        // connection was closed gracefully
                        printf("Connection with client closed.\n");
                        closesocket(acceptedSocket[i]);
                    }
                    else
                    {
                        // there was an error during recv
                        printf("recv failed with error: %d\n", WSAGetLastError());
                        closesocket(acceptedSocket[i]);
                    }
                }

            }
        }
    } while (1);

    for (int i = 0; i < clientNum; i++)
    {
        iResult = shutdown(acceptedSocket[i], SD_SEND);
        if (iResult == SOCKET_ERROR)
        {
            printf("shutdown failed with error: %d\n", WSAGetLastError());
            closesocket(acceptedSocket[i]);
            WSACleanup();
            return 1;
        }
        closesocket(acceptedSocket[i]);
    }


    // cleanup
    closesocket(listenSocket);
    WSACleanup();

    return 0;
}

bool InitializeWindowsSockets()
{
    WSADATA wsaData;
    // Initialize windows sockets library for this process
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}

//thread koji cita iz queua
//algoritam neki ili nesto, sta ovde raditi ?

DWORD WINAPI statusDequeueThread(LPVOID lpParam)
{
    PARAMS* params = (PARAMS*)lpParam;
    while (true)
    {
        Sleep(10000);
        while (*params->glava != NULL)  //dok god nije prazan queue
        {
            CLIENT_MESSAGE vrednost = Dequeue(&(*params->glava), *params->cs);   //uzimaj sa queue-a

            int iResult = send(acceptedSocket2[0], (char*)&vrednost, sizeof(CLIENT_MESSAGE), 0);

            if (iResult == SOCKET_ERROR)
            {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(acceptedSocket2[0]);
                WSACleanup();
                return 1;
            }


        }
    }

    return 0;
}

DWORD WINAPI analogDequeueThread(LPVOID lpParam)
{
    PARAMS* params = (PARAMS*)lpParam;
    while (true)
    {
        Sleep(10000);
        while (*params->glava != NULL)
        {
            CLIENT_MESSAGE vrednost = Dequeue(&(*params->glava), *params->cs);
            //javi svim subscriberima da se nesto promenilo



        }
    }

    return 0;
}