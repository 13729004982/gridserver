#ifndef _SERVER_H_
#define _SERVER_H_

#include "mysqldatabase.h"
#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


class Server 
{
public:
    Server(const std::string& host, const std::string& user, const std::string& password, const std::string& db);
    ~Server();
    MySQLDatabase* m_db;
    void initAndRun();
    

private:
    
    int m_socket;

    static int send_response(struct MHD_Connection *connection, const std::string& response, int status_code);
    static int iterate_post(void *coninfo_cls, enum MHD_ValueKind kind, const char *key, const char *filename,const char *content_type, const char *transfer_encoding, const char *data, uint64_t off, size_t size);
    static int handle_request(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls);
    void reconnectDB(MySQLDatabase* db);
};
#endif