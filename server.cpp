#include <sys/socket.h>
#include <cstdio>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <thread>
#include <sstream>
#include <unordered_map>
#include <fstream>
#include <functional>

// test
#include <chrono>

using std::function;
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

unordered_map<string, function<void(int)>> routes;

void send_error(int client, int code, const string &message)
{
    string body = message + "\n";
    string response =
        "HTTP/1.1 " +
        std::to_string(code) +
        " " +
        message +
        "\r\n"
        "Content-Length: " +
        std::to_string(body.size()) +
        "\r\n"
        "Connection: close\r\n"
        "\r\n" +
        body;

    send(client, response.c_str(), response.size(), 0);
}

void send_response(int client, const string &body, const string &status = "200 OK", const string &type = "text/plain")
{
    string response =
        "HTTP/1.1 " +
        status +
        "\r\n"
        "Content-Type: " +
        type +
        "\r\n"
        "Content-Length: " +
        std::to_string(body.size()) +
        "\r\n"
        "Connection: close\r\n"
        "\r\n" +
        body;

    ssize_t nSend = send(client, response.c_str(), response.size(), 0);

    if (nSend < 0)
    {
        printf("Failed to send to Client %d \n", client);
    }
    else
    {
        printf("Response sent to Client %d \n", client);
    }
}

void serve_index(int client)
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

    send_response(client, response_str, "200 OK", "text/html");
}

void serve_hello(int client)
{
    send_response(client, "<h1>Hello</h1>", "200 OK", "text/html");
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

    if (method.empty() || path.empty() || version.empty())
    {
        send_error(client_connection, 400, "Bad Request");
        return;
    }

    if (routes.count(path) == 0)
    {
        send_error(client_connection, 404, "Not Found");
        return;
    }

    if (method != "GET" && method != "POST")
    {
        send_error(client_connection, 405, "Method Not Allowed");
        return;
    }

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
    try
    {
        if (path == "/")
        {
            serve_index(client_connection);
        }
        else if (path == "/hello")
        {
            serve_hello(client_connection);
        }
    }
    catch (...)
    {
        send_error(client_connection, 500, "Internal Server Error");
    }

    close(client_connection);
}

int main()
{
    // route map
    routes["/"] = serve_index;
    routes["/hello"] = serve_hello;

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
