#include <sys/socket.h>
#include <cstdio>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <thread>
#include <sstream>
#include <unordered_map>
#include <fstream>

// test
#include <chrono>

using std::getline;
using std::ifstream;
using std::istringstream;
using std::string;
using std::stringstream;
using std::thread;
using std::unordered_map;

// test
using std::chrono::seconds;
using std::this_thread::sleep_for;

string serve_index()
{
    ifstream f("./www/index.html");
    stringstream file_content;

    if (f.is_open())
    {
        file_content << f.rdbuf();
    }
    else
    {
        file_content << "<h1>Index file not found</h1>";
    }

    string response_str = file_content.str();

    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/html\r\n"
           "Content-Length: " +
           std::to_string(response_str.size()) +
           "\r\n"
           "Connection: close\r\n"
           "\r\n" +
           response_str;
}

string serve_hello()
{
    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/html\r\n"
           "Content-Length: 15\r\n"
           "Connection: close\r\n"
           "\r\n"
           "<h1>Hello!</h1>";
}

string serve_404()
{
    return "HTTP/1.1 404 Not Found\r\n"
           "Content-Type: text/html\r\n"
           "Content-Length: 22\r\n"
           "Connection: close\r\n"
           "\r\n"
           "<h1>404 Not Found</h1>";
}

unordered_map<string, string> parse_headers(const string &header_string)
{
    unordered_map<string, string> headers;

    istringstream iss(header_string);
    string cur_line;

    // request line
    getline(iss, cur_line);

    // parse headers
    while (getline(iss, cur_line))
    {
        // end of header
        if (cur_line == "\r" || cur_line.empty())
        {
            break;
        }

        // remove \r
        if (!cur_line.empty() && cur_line.back() == '\r')
        {
            cur_line.pop_back();
        }

        // parse
        size_t colon = cur_line.find(':');
        if (colon == string::npos)
        {
            continue;
        }

        string key = cur_line.substr(0, colon);
        string value = cur_line.substr(colon + 2);

        headers[key] = value;
    }

    return headers;
}

void handle_client(int client_connection)
{
    // test
    // sleep_for(seconds(10));

    // read
    string request;
    char buffer[4096];

    while (true)
    {
        ssize_t bytes_received = recv(client_connection, buffer, sizeof(buffer), 0);

        if (bytes_received <= 0)
        {
            break;
        }

        request.append(buffer, bytes_received);

        // printf("%d bytes received from Client %d \n", bytes_received, client_connection);
        if (request.find("\r\n\r\n") != std::string::npos)
        {
            break;
        }
    }

    // parse the http request

    // GET /hello HTTP/1.1\r\n
    // Host: localhost:8080\r\n
    // User-Agent: curl/7.85.0\r\n
    // Accept: */*\r\n
    // Connection: close\r\n
    // \r\n
    size_t header_end = request.find("\r\n\r\n");
    string header = request.substr(0, header_end);
    string body = request.substr(header_end + 4);

    unordered_map<string, string> headers = parse_headers(header);

    size_t pos = header.find("\r\n");
    string request_line = header.substr(0, pos);

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

    // read in rest of body
    int body_len = 0;
    if (headers.find("Content-Length") != headers.end())
    {
        body_len = stoi(headers["Content-Length"]);
    }

    while (body.size() < body_len)
    {
        ssize_t bytes_received = recv(client_connection, buffer, sizeof(buffer), 0);

        if (bytes_received <= 0)
        {
            break;
        }

        body.append(buffer, bytes_received);
    }

    // construct the http response
    string response;

    if (path == "/")
    {
        response = serve_index();
    }
    else if (path == "/hello")
    {
        response = serve_hello();
    }
    else
    {
        response = serve_404();
    }

    ssize_t nSend = send(client_connection, response.c_str(), response.size(), 0);

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
