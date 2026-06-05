#include "db_connection.h"
#include <iostream>
using namespace std;

MYSQL *DBConnection::connect()
{
    MYSQL *conn = mysql_init(nullptr);

    if (conn == nullptr)
    {
        cout << "mysql_init failed\n";
        return nullptr;
    }

    MYSQL *result = mysql_real_connect(
        conn,
        "127.0.0.1",
        "root",
        "",
        "cartforge",
        3306,
        nullptr,
        0);

    if (result == nullptr)
    {
        cout << "Connection failed: " << mysql_error(conn) << endl;
        mysql_close(conn);
        return nullptr;
    }

    return conn;
}