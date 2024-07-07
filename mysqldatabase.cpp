#include "mysqldatabase.h"

MySQLDatabase::MySQLDatabase(const std::string& host, const std::string& user, const std::string& password, const std::string& db) 
{
    driver = sql::mysql::get_mysql_driver_instance();
    con = driver->connect(host, user, password);
    con->setSchema(db);
}

MySQLDatabase::~MySQLDatabase() 
{
    delete con;
}

MySQLDatabase::sql::ResultSet* query(const std::string& query) 
{
    sql::Statement* stmt = con->createStatement();
    sql::ResultSet* res = stmt->executeQuery(query);
    delete stmt;
    return res;
}

MySQLDatabase::void update(const std::string& query) 
{
    sql::Statement* stmt = con->createStatement();
    stmt->executeUpdate(query);
    delete stmt;
}