#include "user.h"
#include "db_connection.h"
#include "utils.h"

#include <iostream>
#include <string>

using namespace std;

User::User(MYSQL *connection)
{
    conn = connection;
}

void User::addUser()
{
    string name = util::readText("Enter user name: ", 100);
    string email = util::readEmail("Enter email: ");

    Statement stmt(conn, "INSERT INTO users (name, email) VALUES (?, ?)");

    if (stmt.execute({name, email}))
        cout << "User added successfully (ID " << stmt.insertId() << ").\n";
    else if (stmt.errorCode() == dberr::DUPLICATE_ENTRY)
        cout << "That e-mail address is already registered.\n";
    else
        cout << "Error: " << stmt.errorMessage() << endl;
}

void User::viewUsers()
{
    Statement stmt(conn, "SELECT user_id, name, email FROM users ORDER BY user_id");

    if (!stmt.execute())
    {
        cout << "Error: " << stmt.errorMessage() << endl;
        return;
    }

    util::printTable({"ID", "Name", "Email"}, stmt.fetchAll());
}
