#ifndef DB_CONNECTION_H
#define DB_CONNECTION_H

#include <mysql.h>

class DBConnection
{
public:
    MYSQL *connect();
};

#endif