#ifndef USER_H
#define USER_H

#include <mysql.h>

class User
{
private:
    MYSQL *conn;

public:
    User(MYSQL *connection);

    void addUser();
    void viewUsers();
};

#endif