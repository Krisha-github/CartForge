#include "db_connection.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace std;

// ---------------------------------------------------------------- DBConfig

static string envOr(const char *name, const string &fallback)
{
    const char *value = getenv(name);
    return (value != nullptr) ? string(value) : fallback;
}

DBConfig DBConfig::fromEnvironment()
{
    DBConfig cfg;
    cfg.host = envOr("CARTFORGE_DB_HOST", cfg.host);
    cfg.user = envOr("CARTFORGE_DB_USER", cfg.user);
    cfg.password = envOr("CARTFORGE_DB_PASSWORD", cfg.password);
    cfg.database = envOr("CARTFORGE_DB_NAME", cfg.database);

    const char *port = getenv("CARTFORGE_DB_PORT");
    if (port != nullptr)
    {
        long p = strtol(port, nullptr, 10);
        if (p > 0 && p < 65536)
            cfg.port = static_cast<unsigned int>(p);
    }
    return cfg;
}

// ------------------------------------------------------------ DBConnection

MYSQL *DBConnection::connect()
{
    return connect(DBConfig::fromEnvironment());
}

MYSQL *DBConnection::connect(const DBConfig &cfg)
{
    MYSQL *conn = mysql_init(nullptr);
    if (conn == nullptr)
    {
        cout << "mysql_init failed\n";
        return nullptr;
    }

    mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");

    // CLIENT_FOUND_ROWS: UPDATE/DELETE report rows *matched*, not just changed,
    // so "0 rows" reliably means "no such record".
    MYSQL *result = mysql_real_connect(
        conn,
        cfg.host.c_str(),
        cfg.user.c_str(),
        cfg.password.c_str(),
        cfg.database.c_str(),
        cfg.port,
        nullptr,
        CLIENT_FOUND_ROWS);

    if (result == nullptr)
    {
        cout << "Connection failed: " << mysql_error(conn) << endl;
        mysql_close(conn);
        return nullptr;
    }

    return conn;
}

// --------------------------------------------------------------- Statement

static const size_t kColumnBuffer = 1024; // bytes per result column

Statement::Statement(MYSQL *conn, const string &sql)
{
    stmt_ = mysql_stmt_init(conn);
    if (stmt_ == nullptr)
    {
        errCode_ = 2008; // CR_OUT_OF_MEMORY
        errMsg_ = "Could not allocate statement";
        return;
    }

    if (mysql_stmt_prepare(stmt_, sql.c_str(), static_cast<unsigned long>(sql.size())) != 0)
    {
        captureError();
        return;
    }
    prepared_ = true;
}

Statement::~Statement()
{
    if (stmt_ != nullptr)
        mysql_stmt_close(stmt_);
}

void Statement::captureError()
{
    errCode_ = mysql_stmt_errno(stmt_);
    errMsg_ = mysql_stmt_error(stmt_);
}

bool Statement::execute(const vector<string> &params)
{
    errCode_ = 0;
    errMsg_.clear();

    if (!prepared_)
        return false;

    if (fields_ > 0)
        mysql_stmt_free_result(stmt_);
    fields_ = 0;

    if (params.size() != mysql_stmt_param_count(stmt_))
    {
        errCode_ = 2034; // CR_PARAMS_NOT_BOUND
        errMsg_ = "Wrong number of parameters for prepared statement";
        return false;
    }

    // Bind parameters (all as strings).
    vector<MYSQL_BIND> binds(params.size());
    vector<unsigned long> lens(params.size());
    for (size_t i = 0; i < params.size(); ++i)
    {
        lens[i] = static_cast<unsigned long>(params[i].size());
        binds[i].buffer_type = MYSQL_TYPE_STRING;
        binds[i].buffer = const_cast<char *>(params[i].data());
        binds[i].buffer_length = lens[i];
        binds[i].length = &lens[i];
    }

    if (!params.empty() && mysql_stmt_bind_param(stmt_, binds.data()) != 0)
    {
        captureError();
        return false;
    }

    if (mysql_stmt_execute(stmt_) != 0)
    {
        captureError();
        return false;
    }

    fields_ = mysql_stmt_field_count(stmt_);
    if (fields_ > 0)
    {
        buffers_.assign(fields_, vector<char>(kColumnBuffer));
        lengths_.assign(fields_, 0);
        nulls_.reset(new bool[fields_]());
        resultBinds_.assign(fields_, MYSQL_BIND());

        for (unsigned int i = 0; i < fields_; ++i)
        {
            resultBinds_[i].buffer_type = MYSQL_TYPE_STRING;
            resultBinds_[i].buffer = buffers_[i].data();
            resultBinds_[i].buffer_length = buffers_[i].size();
            resultBinds_[i].length = &lengths_[i];
            resultBinds_[i].is_null = &nulls_[i];
        }

        if (mysql_stmt_bind_result(stmt_, resultBinds_.data()) != 0 ||
            mysql_stmt_store_result(stmt_) != 0)
        {
            captureError();
            fields_ = 0;
            return false;
        }
    }

    return true;
}

bool Statement::fetch(vector<string> &row)
{
    if (!prepared_ || fields_ == 0)
        return false;

    int rc = mysql_stmt_fetch(stmt_);
    if (rc == MYSQL_NO_DATA)
        return false;
    if (rc == 1)
    {
        captureError();
        return false;
    }
    // rc == MYSQL_DATA_TRUNCATED is tolerated: value is cut to the buffer size.

    row.assign(fields_, "");
    for (unsigned int i = 0; i < fields_; ++i)
    {
        if (!nulls_[i])
        {
            size_t len = min<size_t>(lengths_[i], buffers_[i].size());
            row[i].assign(buffers_[i].data(), len);
        }
    }
    return true;
}

vector<vector<string>> Statement::fetchAll()
{
    vector<vector<string>> rows;
    vector<string> row;
    while (fetch(row))
        rows.push_back(row);
    return rows;
}

unsigned long long Statement::insertId() const
{
    return prepared_ ? mysql_stmt_insert_id(stmt_) : 0;
}

unsigned long long Statement::affectedRows() const
{
    return prepared_ ? mysql_stmt_affected_rows(stmt_) : 0;
}

// ----------------------------------------------------------------- helpers

bool rowExists(MYSQL *conn, const string &sql, const vector<string> &params)
{
    Statement stmt(conn, sql);
    if (!stmt.execute(params))
        return false;
    vector<string> row;
    return stmt.fetch(row);
}
