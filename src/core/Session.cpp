// core/Session.cpp
#include "core/Session.hpp"
#include "utils/utils.hpp"
#include <cstdlib>
#include <cstring>

Session::Session(Database& db)
    : db_(db), userId_(-1), loggedIn_(false) {
    token_ = readCookieToken();
    if (!token_.empty())
        loggedIn_ = validate();
}

// -------------------------------------------------------------
// Read session token from browser cookies
// -------------------------------------------------------------
std::string Session::readCookieToken() const {
    const char* cookieEnv = std::getenv("HTTP_COOKIE");
    if (!cookieEnv)
        return "";
    return getCookieValue(std::string(cookieEnv), "session_token");
}

// -------------------------------------------------------------
// Validate current session token (and refresh last_active)
// -------------------------------------------------------------
bool Session::validate() {
    if (token_.empty())
        return false;

    MYSQL* conn = db_.connection();
    if (!conn)
        return false;

    const char* sql =
        "SELECT u.user_id, u.user_email "
        "FROM sessions s JOIN users u ON s.user_id=u.user_id "
        "WHERE s.session_token=? AND s.last_active > NOW() - INTERVAL 5 MINUTE "
        "LIMIT 1";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt)
        return false;

    if (mysql_stmt_prepare(stmt, sql, std::strlen(sql)) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND param{};
    std::memset(&param, 0, sizeof(param));
    param.buffer_type = MYSQL_TYPE_STRING;
    param.buffer = (char*)token_.c_str();
    param.buffer_length = token_.size();

    mysql_stmt_bind_param(stmt, &param);
    mysql_stmt_execute(stmt);

    MYSQL_BIND result[2];
    std::memset(result, 0, sizeof(result));
    long id = 0;
    char email[128];
    unsigned long len = 0;

    result[0].buffer_type = MYSQL_TYPE_LONG;
    result[0].buffer = &id;
    result[0].is_unsigned = 1;

    result[1].buffer_type = MYSQL_TYPE_STRING;
    result[1].buffer = email;
    result[1].buffer_length = sizeof(email);
    result[1].length = &len;

    mysql_stmt_bind_result(stmt, result);
    bool ok = (mysql_stmt_fetch(stmt) == 0);
    mysql_stmt_close(stmt);

    if (ok) {
        userId_ = id;
        email_ = std::string(email, len);
        loggedIn_ = true;

        // Refresh last_active timestamp
        const char* updateSQL =
            "UPDATE sessions SET last_active=NOW() WHERE session_token=?";
        MYSQL_STMT* upd = mysql_stmt_init(conn);
        if (upd && mysql_stmt_prepare(upd, updateSQL, std::strlen(updateSQL)) == 0) {
            MYSQL_BIND up{};
            std::memset(&up, 0, sizeof(up));
            up.buffer_type = MYSQL_TYPE_STRING;
            up.buffer = (char*)token_.c_str();
            up.buffer_length = token_.size();
            mysql_stmt_bind_param(upd, &up);
            mysql_stmt_execute(upd);
            mysql_stmt_close(upd);
        }
    }
    else {
        loggedIn_ = false;
    }

    return ok;
}

// -------------------------------------------------------------
// Create a new session record in the database
// -------------------------------------------------------------
void Session::create(long uid, const std::string& token, const std::string& ip) {
    userId_ = uid;
    token_ = token;
    loggedIn_ = true;

    MYSQL* conn = db_.connection();
    if (!conn)
        return;

    const char* sql =
        "INSERT INTO sessions (user_id, session_token, ip_address, last_active) "
        "VALUES (?, ?, ?, NOW())";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt)
        return;

    if (mysql_stmt_prepare(stmt, sql, std::strlen(sql)) == 0) {
        MYSQL_BIND params[3];
        std::memset(params, 0, sizeof(params));

        params[0].buffer_type = MYSQL_TYPE_LONG;
        params[0].buffer = &userId_;

        params[1].buffer_type = MYSQL_TYPE_STRING;
        params[1].buffer = (char*)token_.c_str();
        params[1].buffer_length = token_.size();

        params[2].buffer_type = MYSQL_TYPE_STRING;
        params[2].buffer = (char*)ip.c_str();
        params[2].buffer_length = ip.size();

        mysql_stmt_bind_param(stmt, params);
        mysql_stmt_execute(stmt);
    }
    mysql_stmt_close(stmt);
}

// -------------------------------------------------------------
// Destroy current session (logout)
// -------------------------------------------------------------
void Session::destroy() {
    if (token_.empty())
        return;

    MYSQL* conn = db_.connection();
    if (!conn)
        return;

    const char* sql = "DELETE FROM sessions WHERE session_token=?";
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (stmt && mysql_stmt_prepare(stmt, sql, std::strlen(sql)) == 0) {
        MYSQL_BIND p{};
        std::memset(&p, 0, sizeof(p));
        p.buffer_type = MYSQL_TYPE_STRING;
        p.buffer = (char*)token_.c_str();
        p.buffer_length = token_.size();
        mysql_stmt_bind_param(stmt, &p);
        mysql_stmt_execute(stmt);
        mysql_stmt_close(stmt);
    }

    loggedIn_ = false;
    token_.clear();
}
