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

// generic error handler
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

// generic response handler
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

// mime map
string get_content_type(const string &path)
{
    if (path.ends_with(".html"))
        return "text/html";
    if (path.ends_with(".css"))
        return "text/css";
    if (path.ends_with(".js"))
        return "application/javascript";
    if (path.ends_with(".png"))
        return "image/png";
    if (path.ends_with(".jpg"))
        return "image/jpeg";
    if (path.ends_with(".jpeg"))
        return "image/jpeg";
    if (path.ends_with(".gif"))
        return "image/gif";
    return "application/octet-stream";
}

// generic file handler
void serve_static(int client, const string &path)
{
    std::string filepath = "www";

    if (path == "/")
        filepath += "/index.html";
    else
        filepath += path;

    ifstream f(filepath);
    stringstream file_content;

    if (f.is_open())
    {
        file_content << f.rdbuf();
    }
    else
    {
        send_error(client, 404, "Not Found");
    }

    string response_str = file_content.str();

    send_response(client, response_str, "200 OK", get_content_type(filepath));
}

// parse request header
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

// main packet processing
void handle_client(int client_connection)
{
    // read incoming message
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

        // reach end of http request
        if (request.find("\r\n\r\n") != std::string::npos)
        {
            break;
        }
    }

    // parse the http request
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

    // check if valid request
    if (method.empty() || path.empty() || version.empty())
    {
        send_error(client_connection, 400, "Bad Request");
        return;
    }

    if (path.find("..") != string::npos) // malicious path
    {
        send_error(client_connection, 400, "Bad Request");
        return;
    }

    if (method != "GET" && method != "POST" && method != "HEAD")
    {
        send_error(client_connection, 405, "Method Not Allowed");
        return;
    }

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

    // serve the http response
    try
    {
        serve_static(client_connection, path);
    }
    catch (...)
    {
        send_error(client_connection, 500, "Internal Server Error");
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
