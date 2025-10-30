#include "utils.hpp"
#include <iostream>
#include <map>
#include <cstring>
#include <mysql/mysql.h>

// -----------------------------------------------------------------------------
// Helper: runSelectExists — executes SELECT ... WHERE ... LIMIT 1 and returns true if a row exists
// -----------------------------------------------------------------------------
bool runSelectExists(MYSQL* conn, const std::string& sql, MYSQL_BIND* params, unsigned int paramCount) {
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return false;
    if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    if (paramCount && mysql_stmt_bind_param(stmt, params) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    my_ulonglong dummy = 0;
    MYSQL_BIND result{};
    memset(&result, 0, sizeof(result));
    result.buffer_type = MYSQL_TYPE_LONGLONG;
    result.buffer = &dummy;
    result.is_unsigned = 1;

    mysql_stmt_bind_result(stmt, &result);
    mysql_stmt_store_result(stmt);

    bool exists = (mysql_stmt_fetch(stmt) == 0);
    mysql_stmt_close(stmt);
    return exists;
}

// -----------------------------------------------------------------------------
// Helper: execStatement — executes INSERT/UPDATE/DELETE with bound params safely
// -----------------------------------------------------------------------------
bool execStatement(MYSQL* conn, const std::string& sql, MYSQL_BIND* params, unsigned int paramCount) {
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return false;
    if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    if (paramCount && mysql_stmt_bind_param(stmt, params) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    bool success = (mysql_stmt_execute(stmt) == 0);
    mysql_stmt_close(stmt);
    return success;
}

// -----------------------------------------------------------------------------
// Page rendering
// -----------------------------------------------------------------------------
void renderRegisterForm(const std::string& emailPrefill = "", const std::string& errorMsg = "") {
    sendHTMLHeader();
    printHead("Create your account", "auth");
    std::cout
        << "    <section class='card'>\n"
        << "      <h1>Create your account</h1>\n"
        << "      <p class='muted'>Join Team Elevate to start bidding and tracking your favorites.</p>\n";
    if (!errorMsg.empty()) {
        std::cout << "      <div class='error'>" << errorMsg << "</div>\n";
    }
    std::cout
        << "      <form method='post' action='register.cgi'>\n"
        << "        <div>\n"
        << "          <label for='email'>Email</label>\n"
        << "          <input id='email' name='email' type='email' maxlength='100' required value='" << htmlEscape(emailPrefill) << "'>\n"
        << "        </div>\n"
        << "        <div>\n"
        << "          <label for='password'>Password</label>\n"
        << "          <input id='password' name='password' type='password' minlength='8' required>\n"
        << "          <div class='helper'>Use at least 8 characters.</div>\n"
        << "        </div>\n"
        << "        <div>\n"
        << "          <label for='confirm'>Confirm Password</label>\n"
        << "          <input id='confirm' name='confirm' type='password' minlength='8' required>\n"
        << "        </div>\n"
        << "        <button class='btn' type='submit'>Create account</button>\n"
        << "      </form>\n"
        << "      <div class='top-gap helper'>Already have an account? <a href='login.cgi'>Log in</a>.</div>\n"
        << "    </section>\n";
    printTail("auth");
}

void renderRegisterSuccess(const std::string& email, const std::string& sessionToken) {
    std::cout << "Content-Type: text/html\n"
        << "Set-Cookie: session_token=" << sessionToken << "; Path=/; HttpOnly; SameSite=Lax\n\n";
    printHead("Registration Successful", "auth");
    std::cout
        << "    <section class='card'>\n"
        << "      <h1>✓ Registration Successful</h1>\n"
        << "      <div class='success'>Welcome to Team Elevate, " << htmlEscape(email) << ".</div>\n"
        << "      <p class='muted'>You’ll be redirected to the home page momentarily.</p>\n"
        << "      <meta http-equiv='refresh' content='3;url=index.cgi'>\n"
        << "      <div class='top-gap'><a class='btn' href='index.cgi'>Go now</a></div>\n"
        << "    </section>\n";
    printTail("auth");
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------
int main() {
    const char* requestMethod = std::getenv("REQUEST_METHOD");
    if (!requestMethod || std::string(requestMethod) != "POST") {
        renderRegisterForm();
        return 0;
    }

    std::map<std::string, std::string> postData = parsePostData();
    std::string email = postData["email"];
    std::string password = postData["password"];
    std::string confirm = postData["confirm"];

    if (email.empty() || password.empty() || confirm.empty()) {
        renderRegisterForm(email, "All fields are required.");
        return 0;
    }
    if (!isValidEmail(email)) {
        renderRegisterForm(email, "Please enter a valid email address.");
        return 0;
    }
    if (password.length() < 8) {
        renderRegisterForm(email, "Password must be at least 8 characters.");
        return 0;
    }
    if (password != confirm) {
        renderRegisterForm(email, "Passwords do not match.");
        return 0;
    }

    MYSQL* conn = createDBConnection();
    if (!conn) {
        renderRegisterForm(email, "Internal server error. Please try again later.");
        return 0;
    }

    // -------------------------------------------------------------------------
    // Check if email already exists
    // -------------------------------------------------------------------------
    MYSQL_BIND checkParam[1];
    memset(checkParam, 0, sizeof(checkParam));
    checkParam[0].buffer_type = MYSQL_TYPE_STRING;
    checkParam[0].buffer = (char*)email.c_str();
    checkParam[0].buffer_length = email.size();

    if (runSelectExists(conn, "SELECT user_id FROM users WHERE user_email=? LIMIT 1", checkParam, 1)) {
        renderRegisterForm(email, "This email is already registered. Try a different one or log in.");
        closeDBConnection(conn);
        return 0;
    }

    // -------------------------------------------------------------------------
    // Insert new user
    // -------------------------------------------------------------------------
    std::string hashedPassword = hashPassword(password);
    MYSQL_BIND insertParam[2];
    memset(insertParam, 0, sizeof(insertParam));
    insertParam[0].buffer_type = MYSQL_TYPE_STRING;
    insertParam[0].buffer = (char*)email.c_str();
    insertParam[0].buffer_length = email.size();
    insertParam[1].buffer_type = MYSQL_TYPE_STRING;
    insertParam[1].buffer = (char*)hashedPassword.c_str();
    insertParam[1].buffer_length = hashedPassword.size();

    if (!execStatement(conn, "INSERT INTO users (user_email, password_hash, joindate) VALUES (?, ?, NOW())", insertParam, 2)) {
        renderRegisterForm(email, "Internal server error. Please try again later.");
        closeDBConnection(conn);
        return 0;
    }

    unsigned long long newUserId = mysql_insert_id(conn);

    // -------------------------------------------------------------------------
    // Create session record
    // -------------------------------------------------------------------------
    std::string sessionToken = generateSessionToken();
    char* remoteAddr = std::getenv("REMOTE_ADDR");
    std::string ipAddress = remoteAddr ? std::string(remoteAddr) : "unknown";

    MYSQL_BIND sessionParam[3];
    memset(sessionParam, 0, sizeof(sessionParam));
    sessionParam[0].buffer_type = MYSQL_TYPE_LONGLONG;
    sessionParam[0].buffer = &newUserId;
    sessionParam[0].is_unsigned = 1;
    sessionParam[1].buffer_type = MYSQL_TYPE_STRING;
    sessionParam[1].buffer = (char*)sessionToken.c_str();
    sessionParam[1].buffer_length = sessionToken.size();
    sessionParam[2].buffer_type = MYSQL_TYPE_STRING;
    sessionParam[2].buffer = (char*)ipAddress.c_str();
    sessionParam[2].buffer_length = ipAddress.size();

    if (!execStatement(conn, "INSERT INTO sessions (user_id, session_token, login_time, last_active, ip_address) VALUES (?, ?, NOW(), NOW(), ?)", sessionParam, 3)) {
        renderRegisterForm(email, "Internal server error. Please try again later.");
        closeDBConnection(conn);
        return 0;
    }

    closeDBConnection(conn);
    renderRegisterSuccess(email, sessionToken);
    return 0;
}
