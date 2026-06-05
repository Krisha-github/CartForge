#include "user.h"
#include <iostream>
#include <string>
using namespace std;

User::User(MYSQL *connection)
{
    conn = connection;
}

void User::addUser()
{
    string name, email;

    cin.ignore();
    cout << "Enter user name: ";
    getline(cin, name);
    cout << "Enter email: ";
    getline(cin, email);

    string query = "INSERT INTO users (name, email) VALUES ('" + name + "', '" + email + "')";

    if (mysql_query(conn, query.c_str()) == 0)
        cout << "User added successfully.\n";
    else
        cout << "Error: " << mysql_error(conn) << endl;
}

void User::viewUsers()
{
    string query = "SELECT user_id, name, email FROM users";

    if (mysql_query(conn, query.c_str()) != 0)
    {
        cout << "Error: " << mysql_error(conn) << endl;
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;

    cout << "\nID | Name | Email\n";
    cout << "-------------------------\n";

    while ((row = mysql_fetch_row(res)))
    {
        cout << row[0] << " | " << row[1] << " | " << row[2] << endl;
    }

    mysql_free_result(res);
}