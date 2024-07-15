#ifndef _MYSQL_DATABASE_H_
#define _MYSQL_DATABASE_H_
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <iostream>
#include <string>

class MySQLDatabase {
public:
    MySQLDatabase(const std::string& host, const std::string& user, const std::string& password, const std::string& db);

    ~MySQLDatabase();

    sql::ResultSet* query(const std::string& query);

    void update(const std::string& query);

    void reconnect();
private:
    sql::Driver* driver;
    sql::Connection* con;
    std::string m_host;
    std::string m_user;
    std::string m_password;
    std::string m_db;
};
#endif