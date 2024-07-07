#include <iostream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <json/json.h>
#include "server.h"

#define PORT 8080

Server::Server(const std::string& host, const std::string& user, const std::string& password, const std::string& db) 
{
    m_db = new MySQLDatabase(host, user, password, db);
}

Server::~Server() 
{
    delete db;
}

void Server::initAndRun() 
{
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) 
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (true) {
        if ((m_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) 
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        handleClient(m_socket, m_db);
    }
}

void Server::handleClient(MySQLDatabase* db) 
{
    char buffer[1024] = {0};
    read(m_socket, buffer, 1024);

    Json::Reader reader;
    Json::Value request;
    reader.parse(buffer, request);

    std::string action = request["action"].asString();
    std::string response;

    if (action == "get") 
    {
        std::string id = request["id"].asString();
        sql::ResultSet* res = db.query("SELECT * FROM machines WHERE id = '" + id + "'");
        Json::Value result;
        if (res->next()) 
        {
            result["id"] = res->getString("id");
            result["ip"] = res->getString("ip");
            result["port"] = res->getInt("port");
            result["voltage"] = res->getDouble("voltage");
            result["daily_usage"] = res->getDouble("daily_usage");
            result["monthly_usage"] = res->getDouble("monthly_usage");
            result["last_update"] = res->getString("last_update");
            result["last_query"] = res->getString("last_query");
        }
        delete res;
        Json::StreamWriterBuilder writer;
        response = Json::writeString(writer, result);
    } 
    else if (action == "update") 
    {
        std::string id = request["id"].asString();
        std::string ip = request["ip"].asString();
        int port = request["port"].asInt();
        double voltage = request["voltage"].asDouble();
        double daily_usage = request["daily_usage"].asDouble();
        double monthly_usage = request["monthly_usage"].asDouble();
        std::string last_update = request["last_update"].asString();
        std::string last_query = request["last_query"].asString();

        std::string query = "UPDATE machines SET ip='" + ip + "', port=" + std::to_string(port) + 
                            ", voltage=" + std::to_string(voltage) + ", daily_usage=" + std::to_string(daily_usage) + 
                            ", monthly_usage=" + std::to_string(monthly_usage) + ", last_update='" + last_update + 
                            "', last_query='" + last_query + "' WHERE id='" + id + "'";
        db.update(query);
        response = "{\"status\":\"success\"}";
    }

    send(m_socket, response.c_str(), response.size(), 0);
    close(m_socket);
}