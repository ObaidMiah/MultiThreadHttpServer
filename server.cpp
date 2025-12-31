#include <sys/socket.h>
#include <cstdio>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <thread>
#include <sstream>

// test
#include <chrono>

using std::istringstream;
using std::string;
using std::thread;

// test
using std::chrono::seconds;
using std::this_thread::sleep_for;

void handle_client(int client_connection)
{
    // test
    // sleep_for(seconds(10));

    // read
    char buffer[4096];
    int bytes_received = recv(client_connection, buffer, sizeof(buffer), 0);

    if (bytes_received <= 0)
    {
        close(client_connection);
        return;
    }

    printf("%d bytes received from Client %d \n", bytes_received, client_connection);

    // parse the http request

    // GET /hello HTTP/1.1\r\n
    // Host: localhost:8080\r\n
    // User-Agent: curl/7.85.0\r\n
    // Accept: */*\r\n
    // Connection: close\r\n
    // \r\n

    string httpRequest(buffer, bytes_received);

    size_t pos = httpRequest.find("\r\n");
    string request_line = httpRequest.substr(0, pos);

    istringstream iss(request_line);
    string method, path, version;
    iss >> method;
    iss >> path;
    iss >> version;

    // print http request
    // if (bytes_received > 0)
    // {
    //    printf("%.*s\n", bytes_received, buffer);
    // }
    // */

    // construct the http response
    string response_body;

    if (path == "/")
    {
        response_body = "Welcome!";
    }
    else if (path == "/hello")
    {
        response_body = "Hello Word!";
    }
    else
    {
        response_body = "404 Not Found";
    }

    string httpResponse = "HTTP/1.1 200 OK\r\n" // Protocol + status code + reason
                          "Content-Length: " +
                          std::to_string(response_body.size()) + "\r\n" + // bytes in body
                          "\r\n" +                                        // end of headers
                          response_body;                                  // body

    ssize_t nSend = send(client_connection, httpResponse.c_str(), httpResponse.size(), 0);

    if (nSend < 0)
    {
        printf("Failed to send to Client %d \n", client_connection);
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
