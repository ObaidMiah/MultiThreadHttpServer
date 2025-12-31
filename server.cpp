#include <sys/socket.h>
#include <cstdio>
#include <netinet/in.h>
#include <string>
#include <unistd.h>

using std::string;

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
        printf("Failed to bing socket \n");
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

        // read
        char buffer[4096];
        int nRecv = recv(clientConnection, buffer, sizeof(buffer), 0);

        printf("%d bytes received \n", nRecv);

        string response = "HTTP/1.1 200 OK\r\n"   // Protocol + status code + reason
                          "Content-Length: 5\r\n" // bytes in body
                          "\r\n"                  // end of headers
                          "Hello";                // body

        ssize_t nSend = send(clientConnection, response.c_str(), response.size(), 0);

        if (nSend < 0)
        {
            printf("Send failed\n");
        }

        close(clientConnection);
    }

    // close
}