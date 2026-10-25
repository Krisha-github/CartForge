#ifndef DB_CONNECTION_H
#define DB_CONNECTION_H

#ifdef _WIN32
#include <winsock2.h> // mysql.h needs the socket types on Windows
#endif
#include <mysql.h>

#include <memory>
#include <string>
#include <vector>

// MySQL server error codes used by the application.
namespace dberr
{
    const unsigned int DUPLICATE_ENTRY = 1062;      // ER_DUP_ENTRY
    const unsigned int LOCK_WAIT_TIMEOUT = 1205;    // ER_LOCK_WAIT_TIMEOUT
    const unsigned int DEADLOCK = 1213;             // ER_LOCK_DEADLOCK
    const unsigned int ROW_IS_REFERENCED = 1451;    // ER_ROW_IS_REFERENCED_2
    const unsigned int NO_REFERENCED_ROW = 1452;    // ER_NO_REFERENCED_ROW_2
    const unsigned int CHECK_VIOLATED = 3819;       // ER_CHECK_CONSTRAINT_VIOLATED
}

// Connection settings. Defaults can be overridden with environment variables:
//   CARTFORGE_DB_HOST, CARTFORGE_DB_PORT, CARTFORGE_DB_USER,
//   CARTFORGE_DB_PASSWORD, CARTFORGE_DB_NAME
struct DBConfig
{
    std::string host = "127.0.0.1";
    unsigned int port = 3306;
    std::string user = "root";
    std::string password = "";
    std::string database = "cartforge";

    static DBConfig fromEnvironment();
};

class DBConnection
{
public:
    // Returns an open connection, or nullptr on failure (error already printed).
    MYSQL *connect();
    MYSQL *connect(const DBConfig &config);
};

// RAII wrapper around a MySQL prepared statement.
//
// All parameters are sent as strings (MySQL converts them to the column type),
// and all result columns are read back as strings. Because user input is only
// ever sent as a bound parameter - never pasted into the SQL text - SQL
// injection is not possible.
class Statement
{
public:
    Statement(MYSQL *conn, const std::string &sql);
    ~Statement();
    Statement(const Statement &) = delete;
    Statement &operator=(const Statement &) = delete;

    // Runs the statement with the given parameters. Returns false on error.
    bool execute(const std::vector<std::string> &params = {});

    // Reads the next result row. Returns false when there are no more rows.
    bool fetch(std::vector<std::string> &row);
    std::vector<std::vector<std::string>> fetchAll();

    unsigned long long insertId() const;
    // Rows matched by an UPDATE/DELETE (the connection uses CLIENT_FOUND_ROWS).
    unsigned long long affectedRows() const;

    unsigned int errorCode() const { return errCode_; }
    const std::string &errorMessage() const { return errMsg_; }

private:
    void captureError();

    MYSQL_STMT *stmt_ = nullptr;
    bool prepared_ = false;
    unsigned int fields_ = 0;

    std::vector<MYSQL_BIND> resultBinds_;
    std::vector<std::vector<char>> buffers_;
    std::vector<unsigned long> lengths_;
    std::unique_ptr<bool[]> nulls_;

    unsigned int errCode_ = 0;
    std::string errMsg_;
};

// Convenience helper: true if the query returns at least one row.
bool rowExists(MYSQL *conn, const std::string &sql, const std::vector<std::string> &params);

#endif
