#include <iostream>
#include <string>
#include <cstring>
#include <microhttpd.h>
#include <json/json.h>
#include "server.h"

#define PORT 8080

Server::Server(const std::string& host, const std::string& user, const std::string& password, const std::string& db) 
{
    m_db = new MySQLDatabase(host, user, password, db);
}

Server::~Server() 
{
    delete m_db;
}

static int send_response(struct MHD_Connection *connection, const std::string& response, int status_code) 
{
    struct MHD_Response *mhd_response;
    mhd_response = MHD_create_response_from_buffer(response.size(), (void*)response.c_str(), MHD_RESPMEM_MUST_COPY);
    int ret = MHD_queue_response(connection, status_code, mhd_response);
    MHD_destroy_response(mhd_response);
    return ret;
}

static int handle_request(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) 
{
    Server* server = static_cast<Server*>(cls);
    if (strcmp(method, "POST") != 0) 
    {
        return MHD_NO;
    }

    static int dummy;
    if (&dummy != *con_cls) 
    {
        *con_cls = &dummy;
        return MHD_YES;
    }

    if (*upload_data_size != 0) 
    {
        std::string request_data(upload_data, *upload_data_size);
        *upload_data_size = 0;

        Json::Reader reader;
        Json::Value request;
        reader.parse(request_data, request);

        std::string action = request["action"].asString();
        std::string response;

        if (action == "get") 
        {
            std::string id = request["id"].asString();
            sql::ResultSet* res = server->m_db->query("SELECT * FROM machines WHERE id = '" + id + "'");
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
            server->m_db->update(query);
            response = "{\"status\":\"success\"}";
        }

        return send_response(connection, response, MHD_HTTP_OK);
    }

    return MHD_NO;
}

void Server::initAndRun() 
{
    struct MHD_Daemon *daemon;
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL, &handle_request, this, MHD_OPTION_END);
    if (NULL == daemon) 
    {
        std::cerr << "Failed to start HTTP server" << std::endl;
        return;
    }

    std::cout << "HTTP server running on port " << PORT << std::endl;

    // 保持服务器运作
    getchar();

    MHD_stop_daemon(daemon);
}