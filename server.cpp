#include <iostream>
#include <string>
#include <cstring>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/writer.h>
#include <microhttpd.h>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/exception.h>
#include <thread>
#include <chrono>
#include "server.h"

#define PORT 8080

struct connection_info_struct {
    char *data;
    size_t data_size;
};

Server::Server(const std::string& host, const std::string& user, const std::string& password, const std::string& db) 
{
    try {
        m_db = new MySQLDatabase(host, user, password, db);
    } catch (sql::SQLException &e) {
        std::cerr << "SQLException during MySQLDatabase construction: " << e.what() << std::endl;
        throw;
    } catch (std::exception &e) {
        std::cerr << "Exception during MySQLDatabase construction: " << e.what() << std::endl;
        throw;
    }
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
    if (NULL == *con_cls) 
    {
        struct connection_info_struct *con_info;

        con_info = (connection_info_struct*)malloc(sizeof(struct connection_info_struct));
        if (NULL == con_info) {
            std::cerr << "Failed to allocate memory for connection_info_struct" << std::endl;
            return MHD_NO;
        }

        con_info->data = NULL;
        con_info->data_size = 0;
        *con_cls = (void *)con_info;
        return MHD_YES;
    }

    struct connection_info_struct *con_info = (struct connection_info_struct *)(*con_cls);
    
    if (strcmp(method, "POST") != 0) 
    {
        return MHD_NO;
    }

    if (*upload_data_size != 0) 
    {
        con_info->data = (char *)realloc(con_info->data, con_info->data_size + *upload_data_size + 1);
        if (con_info->data == NULL) {
            std::cerr << "Failed to allocate memory for post data" << std::endl;
            return MHD_NO;
        }
        
        memcpy(&(con_info->data[con_info->data_size]), upload_data, *upload_data_size);
        con_info->data_size += *upload_data_size;
        con_info->data[con_info->data_size] = '\0';

        *upload_data_size = 0;
        return MHD_YES;
    } 
    else 
    {
        std::string request_data = con_info->data;

        // 添加调试信息
        std::cout << "Received data: " << request_data << std::endl;

        Json::Reader reader;
        Json::Value request;
        try {
            if (!reader.parse(request_data, request)) {
                std::cerr << "Failed to parse request data: " << reader.getFormattedErrorMessages() << std::endl;
                return send_response(connection, "Invalid JSON", MHD_HTTP_BAD_REQUEST);
            }

            if (!request["action"].isString() || !request["table"].isString()) {
                return send_response(connection, "Invalid JSON format: 'action' or 'table' is not a string", MHD_HTTP_BAD_REQUEST);
            }

            std::string action = request["action"].asString();
            std::string tableName = request["table"].asString();
            std::string response;

            if (action == "get") 
            {
                std::string time = request["Time"].isString() ? request["Time"].asString() : "";
                if (time.empty()) {
                    return send_response(connection, "Invalid JSON format: 'Time' is missing or not a string", MHD_HTTP_BAD_REQUEST);
                }

                try {
                    sql::ResultSet* res = server->m_db->query("SELECT * FROM " + tableName +" WHERE Time = '" + time + "'");
                    Json::Value result;
                    if (res->next()) 
                    {
                        result["Time"] = Json::Value(res->getString("Time"));
                        result["Voltage"] = Json::Value(static_cast<double>(res->getDouble("Voltage")));
                        result["Current"] = Json::Value(static_cast<double>(res->getDouble("Current")));
                        result["Power_factor"] = Json::Value(static_cast<int>(res->getInt("Power_factor")));
                        result["Frequency"] = Json::Value(static_cast<double>(res->getDouble("Frequency")));
                        result["Active_Power"] = Json::Value(static_cast<double>(res->getDouble("Active_Power")));
                        result["Reactive_Power"] = Json::Value(static_cast<double>(res->getDouble("Reactive_Power")));
                        result["Total_load"] = Json::Value(static_cast<double>(res->getDouble("Total_load")));
                        result["Transformer_loss"] = Json::Value(static_cast<double>(res->getDouble("Transformer_loss")));
                    }
                    delete res;
                    Json::FastWriter writer;
                    response = writer.write(result);
                } catch (sql::SQLException &e) {
                    std::cerr << "SQLException: " << e.what() << std::endl;
                    return send_response(connection, "Database query failed: " + std::string(e.what()), MHD_HTTP_INTERNAL_SERVER_ERROR);
                }
            }

            free(con_info->data);
            free(con_info);
            *con_cls = NULL;

            return send_response(connection, response, MHD_HTTP_OK);
        } catch (const std::exception &e) {
            std::cerr << "Exception: " << e.what() << std::endl;
            free(con_info->data);
            free(con_info);
            *con_cls = NULL;
            return send_response(connection, "Server Error", MHD_HTTP_INTERNAL_SERVER_ERROR);
        }
    }

    return MHD_NO;
}

void Server::initAndRun() 
{
    try {
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
    } catch (const std::exception &e) {
        std::cerr << "Exception during server initialization or run: " << e.what() << std::endl;
    }
}

void Server::reconnectDB(MySQLDatabase* db)
{
    while (true)
    {
        try {
            db->reconnect();
            std::cout << "Database reconnected successfully" << std::endl;
            break;
        } catch (sql::SQLException &e) {
            std::cerr << "SQLException during reconnect: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5)); // 等待5秒后重试
        }
    }
}
