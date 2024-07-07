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

    void initAndRun();
    void handleClient(int socket, MySQLDatabase* db);
};
#endif