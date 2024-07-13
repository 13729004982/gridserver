#ifndef _SERVER_H_
#define _SERVER_H_

#include "mysqldatabase.h"
#include <string>

class Server 
{
public:
    Server(const std::string& host, const std::string& user, const std::string& password, const std::string& db);

    ~Server();

private:
    MySQLDatabase* m_db;
    int m_socket;

    static int send_response(struct MHD_Connection *connection, const std::string& response, int status_code);
    static int handle_request(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls);
    void initAndRun();
};
#endif