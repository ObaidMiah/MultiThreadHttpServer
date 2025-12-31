#include <sys/socket.h>
#include <cstdio>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <thread>

// test
#include <chrono>

using std::string;
using std::thread;

// test
using std::chrono::seconds;
using std::this_thread::sleep_for;

void handle_client(int client_connection)
{
    // test
    sleep_for(seconds(10));

    // read
    char buffer[4096];
    int nRecv = recv(client_connection, buffer, sizeof(buffer), 0);

    printf("%d bytes received from Client %d \n", nRecv, client_connection);

    string response = "HTTP/1.1 200 OK\r\n"   // Protocol + status code + reason
                      "Content-Length: 5\r\n" // bytes in body
                      "\r\n"                  // end of headers
                      "Hello";                // body

    ssize_t nSend = send(client_connection, response.c_str(), response.size(), 0);

    if (nSend < 0)
    {
        printf("Send failed to Client %d \n", client_connection);
    }
    else
    {
        printf("Response sent to Client %d \n", client_connection);
    }

    close(client_connection);
}

int main()
{
    // create socket
    int httpServer = socket(AF_INET, SOCK_STREAM, 0);

    if (httpServer < 0)
    {
        printf("Failed to create socket \n");
        return -1;
    }

    // bind to address / port
    sockaddr_in address{}; // need to zero
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    int bindStatus = bind(httpServer, (sockaddr *)&address, sizeof(address));

    if (bindStatus < 0)
    {
        printf("Failed to bind socket \n");
    }

    // listen
    int listenStatus = listen(httpServer, SOMAXCONN);

    if (listenStatus < 0)
    {
        printf("Listening error \n");
    }

    while (true)
    {
        // accept
        int clientConnection = accept(httpServer, nullptr, nullptr);

        if (clientConnection < 0)
        {
            printf("Failed to connect with client \n");
        }
        else
        {
            printf("Client number %d connected \n", clientConnection);
        }

        // handle client request
        thread(handle_client, clientConnection).detach();
    }
}
