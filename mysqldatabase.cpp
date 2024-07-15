#include "mysqldatabase.h"

MySQLDatabase::MySQLDatabase(const std::string& host, const std::string& user, const std::string& password, const std::string& db): m_host(host), m_user(user), m_password(password), m_db(db) 
{
    driver = sql::mysql::get_mysql_driver_instance();
    con = driver->connect(host, user, password);
    con->setSchema(m_db);
}

MySQLDatabase::~MySQLDatabase() 
{
    delete con;
}

sql::ResultSet*MySQLDatabase::query(const std::string& query) 
{
    sql::Statement* stmt = con->createStatement();
    sql::ResultSet* res = stmt->executeQuery(query);
    delete stmt;
    return res;
}

void MySQLDatabase::update(const std::string& query) 
{
    sql::Statement* stmt = con->createStatement();
    stmt->executeUpdate(query);
    delete stmt;
}

void MySQLDatabase::reconnect()
{
    delete con;
    con = driver->connect(m_host, m_user, m_password);
    con->setSchema(m_db);
}