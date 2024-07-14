#include <iostream>
#include <string>
#include <cstring>
#include <microhttpd.h>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/writer.h>
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

 int Server::send_response(struct MHD_Connection *connection, const std::string& response, int status_code) 
{
    struct MHD_Response *mhd_response;
    mhd_response = MHD_create_response_from_buffer(response.size(), (void*)response.c_str(), MHD_RESPMEM_MUST_COPY);
    int ret = MHD_queue_response(connection, status_code, mhd_response);
    MHD_destroy_response(mhd_response);
    return ret;
}

int Server::handle_request(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) 
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
        std::string tableName = request["table"].asString();
        std::string response;

        if (action == "get") 
        {
            std::string time = request["Time"].asString();
            sql::ResultSet* res = server->m_db->query("SELECT * FROM " + tableName +" WHERE Time = '" + time + "'");
            Json::Value result;
            if (res->next()) 
            {
                result["Time"] = Json::Value(res->getString("Time"));
                result["Voltage"] = Json::Value(static_cast<double>(res->getDouble("Voltage")));
                result["Current"] = Json::Value(static_cast<double>(res->getDouble("Current")));
                result["Power_factor"] = Json::Value(static_cast<int>(res->getInt("Power_factor")));
                result["Frequency"] = Json::Value(static_cast<double>(res->getDouble("daily_usage")));
                result["Active_Power"] = Json::Value(static_cast<double>(res->getDouble("Active_Power")));
                result["Reactive_Power"] = Json::Value(static_cast<double>(res->getDouble("Reactive_Power")));
                result["Total_load"] = Json::Value(static_cast<double>(res->getDouble("Total_load")));
                result["Transformer_loss"] = Json::Value(static_cast<double>(res->getDouble("Transformer_loss")));
            }
            delete res;
            Json::FastWriter writer;
            response = writer.write(result);
        } 

        return send_response(connection, response, MHD_HTTP_OK);
    }

    return MHD_NO;
}

void Server::initAndRun() 
{
    struct MHD_Daemon *daemon;
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL, &Server::handle_request, this, MHD_OPTION_END);
    if (NULL == daemon) 
    {
        std::cerr << "Failed to start HTTP server" << std::endl;
        return;
    }

    std::cout << "HTTP server running on port " << PORT << std::endl;

    getchar();

    MHD_stop_daemon(daemon);
}