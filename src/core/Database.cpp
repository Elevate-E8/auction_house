#include "Database.hpp"
#include <iostream>
#include <cstdlib>
#include <stdexcept>

// =============================================================
// Database — Team Elevate Auctions
// Handles safe MySQL connection initialization and cleanup.
// =============================================================

Database::Database(const std::string& host,
    const std::string& user,
    const std::string& pass,
    const std::string& db,
    unsigned int port)
    : conn_(nullptr)
{
    conn_ = mysql_init(nullptr);
    if (!conn_) {
        throw std::runtime_error("MySQL initialization failed.");
    }

    // Optional: enable automatic reconnect
    my_bool reconnect = 1;
    mysql_options(conn_, MYSQL_OPT_RECONNECT, &reconnect);

    // ---------------------------------------------------------
    // Try to connect using provided credentials
    // ---------------------------------------------------------
    if (!mysql_real_connect(conn_,
        host.c_str(),
        user.c_str(),
        pass.c_str(),
        db.c_str(),
        port, nullptr, 0)) {
        std::string err = mysql_error(conn_);
        mysql_close(conn_);
        conn_ = nullptr;
        throw std::runtime_error("DB connection failed: " + err);
    }
}

// -------------------------------------------------------------
// Destructor — closes the connection safely
// -------------------------------------------------------------
Database::~Database() {
    if (conn_) {
        mysql_close(conn_);
        conn_ = nullptr;
    }
}

// -------------------------------------------------------------
// Execute a parameterized SQL statement with optional binds
// -------------------------------------------------------------
bool Database::execute(const std::string& sql,
    MYSQL_BIND* params,
    unsigned int count)
{
    if (!conn_) return false;

    MYSQL_STMT* stmt = mysql_stmt_init(conn_);
    if (!stmt) return false;

    if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    if (params && count > 0) {
        if (mysql_stmt_bind_param(stmt, params) != 0) {
            mysql_stmt_close(stmt);
            return false;
        }
    }

    bool success = (mysql_stmt_execute(stmt) == 0);
    mysql_stmt_close(stmt);
    return success;
}

// -------------------------------------------------------------
// Getter for internal MYSQL connection
// -------------------------------------------------------------
MYSQL* Database::connection() const noexcept {
    return conn_;
}
