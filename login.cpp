#include "utils.hpp"
#include <iostream>
#include <map>
#include <string>
#include <mysql/mysql.h>
#include <cstdlib>
#include <cstring>

// -------------------- Login Form --------------------
void renderLoginForm(const std::string& errorMsg = "") {
    sendHTMLHeader();
    printHead("Login · Team Elevate", "auth");

    std::cout
        << "    <section class='card'>\n"
        << "      <h1>Welcome back</h1>\n"
        << "      <p class='muted'>Log in to continue bidding.</p>\n";
    if (!errorMsg.empty()) {
        std::cout << "      <div class='error'>" << errorMsg << "</div>\n";
    }
    std::cout
        << "      <form method='post' action='/~elevate/cgi/login.cgi'>\n"
        << "        <div>\n"
        << "          <label for='email'>Email</label>\n"
        << "          <input id='email' name='email' type='email' maxlength='100' required autocomplete='email'>\n"
        << "        </div>\n"
        << "        <div>\n"
        << "          <label for='password'>Password</label>\n"
        << "          <input id='password' name='password' type='password' minlength='8' required autocomplete='current-password'>\n"
        << "        </div>\n"
        << "        <button class='btn' type='submit'>Log In</button>\n"
        << "      </form>\n"
        << "      <div class='top-gap helper'>Don't have an account? <a href='/~elevate/cgi/register.cgi'>Create one</a>.</div>\n"
        << "    </section>\n";
    printTail("auth");
}

// -------------------- Login Success --------------------
void renderLoginSuccess(const std::string& email, const std::string& sessionToken) {
    std::cout << "Content-Type: text/html\n" 
        << "Set-Cookie: session_token=" << sessionToken << "; Path=/; HttpOnly; SameSite=Lax\n\n";

    printHead("Login Successful", "auth");
    std::cout
        << "    <section class='card' role='status' aria-live='polite'>\n"
        << "      <h1>✓ Login successful</h1>\n"
        << "      <div class='success'>Signed in as <strong>" << htmlEscape(email) << "</strong>.</div>\n"
        << "      <p class='muted'>Redirecting to the homepage…</p>\n"
        << "      <meta http-equiv='refresh' content='2;url=/~elevate/cgi/index.cgi'>\n"
        << "      <a class='btn' href='/~elevate/cgi/index.cgi'>Go to Home</a>\n"
        << "    </section>\n";
    printTail("auth");
}

// -------------------- Main --------------------
int main() {
    const char* requestMethod = std::getenv("REQUEST_METHOD");

    // -------------------- GET Request --------------------
    if (!requestMethod || std::string(requestMethod) != "POST") {
        renderLoginForm();
        return 0;
    }

    // -------------------- POST Request --------------------
    std::map<std::string, std::string> postData = parsePostData();
    std::string email = postData["email"];
    std::string password = postData["password"];

    if (email.empty() || password.empty()) {
        renderLoginForm("Email and password are required.");
        return 0;
    }

    MYSQL* conn = createDBConnection();
    if (!conn) {
        renderLoginForm("Database connection failed. Please try again later.");
        return 0;
    }

    std::string hashedPassword = hashPassword(password);
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    const char* sql = "SELECT user_id FROM users WHERE user_email=? AND password_hash=? LIMIT 1";

    if (!stmt || mysql_stmt_prepare(stmt, sql, std::strlen(sql)) != 0) {
        renderLoginForm("Internal server error.");
        closeDBConnection(conn);
        return 0;
    }

    MYSQL_BIND params[2];
    std::memset(params, 0, sizeof(params));

    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = (char*)email.c_str();
    params[0].buffer_length = email.size();

    params[1].buffer_type = MYSQL_TYPE_STRING;
    params[1].buffer = (char*)hashedPassword.c_str();
    params[1].buffer_length = hashedPassword.size();

    if (mysql_stmt_bind_param(stmt, params) != 0 || mysql_stmt_execute(stmt) != 0) {
        renderLoginForm("Internal server error.");
        mysql_stmt_close(stmt);
        closeDBConnection(conn);
        return 0;
    }

    MYSQL_BIND result{};
    my_ulonglong userId = 0;
    result.buffer_type = MYSQL_TYPE_LONGLONG;
    result.buffer = &userId;
    result.is_unsigned = 1;

    if (mysql_stmt_bind_result(stmt, &result) != 0 ||
        mysql_stmt_store_result(stmt) != 0) {
        renderLoginForm("Internal server error.");
        mysql_stmt_close(stmt);
        closeDBConnection(conn);
        return 0;
    }

    int fetchStatus = mysql_stmt_fetch(stmt);
    mysql_stmt_close(stmt);

    if (fetchStatus != 0) {
        renderLoginForm("Invalid email or password.");
        closeDBConnection(conn);
        return 0;
    }

    // -------------------- Valid credentials --------------------
    std::string sessionToken = generateSessionToken();
    char* remoteAddr = std::getenv("REMOTE_ADDR");
    std::string ipAddress = remoteAddr ? std::string(remoteAddr) : "unknown";

    MYSQL_STMT* insertStmt = mysql_stmt_init(conn);
    const char* insertSql = "INSERT INTO sessions (user_id, session_token, login_time, last_active, ip_address) "
        "VALUES (?, ?, NOW(), NOW(), ?)";

    if (insertStmt && mysql_stmt_prepare(insertStmt, insertSql, std::strlen(insertSql)) == 0) {
        MYSQL_BIND insertParams[3];
        std::memset(insertParams, 0, sizeof(insertParams));

        insertParams[0].buffer_type = MYSQL_TYPE_LONGLONG;
        insertParams[0].buffer = &userId;
        insertParams[0].is_unsigned = 1;

        insertParams[1].buffer_type = MYSQL_TYPE_STRING;
        insertParams[1].buffer = (char*)sessionToken.c_str();
        insertParams[1].buffer_length = sessionToken.size();

        insertParams[2].buffer_type = MYSQL_TYPE_STRING;
        insertParams[2].buffer = (char*)ipAddress.c_str();
        insertParams[2].buffer_length = ipAddress.size();

        mysql_stmt_bind_param(insertStmt, insertParams);
        mysql_stmt_execute(insertStmt);
        mysql_stmt_close(insertStmt);
    }

    closeDBConnection(conn);

    renderLoginSuccess(email, sessionToken);
    return 0;
}
