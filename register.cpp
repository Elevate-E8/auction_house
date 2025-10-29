#include "utils.hpp"
#include <iostream>
#include <map>
#include <cstring>

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
        << "      <form method='post' action='/~elevate/cgi/register.cgi'>\n"
        << "        <div>\n"
        << "          <label for='email'>Email</label>\n"
        << "          <input id='email' name='email' type='email' maxlength='100' required value='" << emailPrefill << "'>\n"
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
        << "      <div class='top-gap helper'>Already have an account? <a href='/~elevate/cgi/login.cgi'>Log in</a>.</div>\n"
        << "    </section>\n";
    printTail("auth");
}

void renderRegisterSuccess(const std::string& email, const std::string& sessionToken) {
    std::cout << "Content-Type: text/html\n" << "Set-Cookie: session_token=" << sessionToken << "; Path=/; HttpOnly; SameSite=Lax\n\n";
    printHead("Registration Successful", "auth");
    std::cout
        << "    <section class='card'>\n"
        << "      <h1>✓ Registration Successful</h1>\n"
        << "      <div class='success'>Welcome to Team Elevate, " << email << ".</div>\n"
        << "      <p class='muted'>You’ll be redirected to the home page momentarily.</p>\n"
        << "      <meta http-equiv='refresh' content='3;url=/~elevate/cgi/index.cgi'>\n"
        << "      <div class='top-gap'><a class='btn' href='/~elevate/cgi/index.cgi'>Go now</a></div>\n"
        << "    </section>\n";
    printTail("auth");
}

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

    MYSQL_STMT* checkStmt = mysql_stmt_init(conn);
    const char* checkSql = "SELECT user_id FROM users WHERE user_email=? LIMIT 1";
    if (!checkStmt || mysql_stmt_prepare(checkStmt, checkSql, std::strlen(checkSql)) != 0) {
        renderRegisterForm(email, "Internal server error. Please try again later.");
        closeDBConnection(conn);
        return 0;
    }

    MYSQL_BIND checkParam[1];
    std::memset(checkParam, 0, sizeof(checkParam));
    checkParam[0].buffer_type = MYSQL_TYPE_STRING;
    checkParam[0].buffer = (char*)email.c_str();
    checkParam[0].buffer_length = email.size();

    if (mysql_stmt_bind_param(checkStmt, checkParam) != 0 || mysql_stmt_execute(checkStmt) != 0) {
        renderRegisterForm(email, "Internal server error. Please try again later.");
        mysql_stmt_close(checkStmt);
        closeDBConnection(conn);
        return 0;
    }

    MYSQL_BIND checkResult;
    std::memset(&checkResult, 0, sizeof(checkResult));
    my_ulonglong userIdDummy = 0;
    checkResult.buffer_type = MYSQL_TYPE_LONGLONG;
    checkResult.buffer = &userIdDummy;
    checkResult.is_unsigned = 1;

    if (mysql_stmt_bind_result(checkStmt, &checkResult) != 0 || mysql_stmt_store_result(checkStmt) != 0) {
        renderRegisterForm(email, "Internal server error. Please try again later.");
        mysql_stmt_close(checkStmt);
        closeDBConnection(conn);
        return 0;
    }

    int fetchStatus = mysql_stmt_fetch(checkStmt);
    mysql_stmt_close(checkStmt);

    if (fetchStatus == 0) {
        renderRegisterForm(email, "This email is already registered. Try a different one or log in.");
        closeDBConnection(conn);
        return 0;
    }

    std::string hashedPassword = hashPassword(password);
    MYSQL_STMT* insertStmt = mysql_stmt_init(conn);
    const char* insertSql = "INSERT INTO users (user_email, password_hash, joindate) VALUES (?, ?, NOW())";
    if (!insertStmt || mysql_stmt_prepare(insertStmt, insertSql, std::strlen(insertSql)) != 0) {
        renderRegisterForm(email, "Internal server error. Please try again later.");
        closeDBConnection(conn);
        return 0;
    }

    MYSQL_BIND insertParam[2];
    std::memset(insertParam, 0, sizeof(insertParam));
    insertParam[0].buffer_type = MYSQL_TYPE_STRING;
    insertParam[0].buffer = (char*)email.c_str();
    insertParam[0].buffer_length = email.size();

    insertParam[1].buffer_type = MYSQL_TYPE_STRING;
    insertParam[1].buffer = (char*)hashedPassword.c_str();
    insertParam[1].buffer_length = hashedPassword.size();

    if (mysql_stmt_bind_param(insertStmt, insertParam) != 0 || mysql_stmt_execute(insertStmt) != 0) {
        renderRegisterForm(email, "Internal server error. Please try again later.");
        mysql_stmt_close(insertStmt);
        closeDBConnection(conn);
        return 0;
    }

    unsigned long long newUserId = mysql_stmt_insert_id(insertStmt);
    mysql_stmt_close(insertStmt);

    std::string sessionToken = generateSessionToken();
    char* remoteAddr = std::getenv("REMOTE_ADDR");
    std::string ipAddress = remoteAddr ? std::string(remoteAddr) : "unknown";

    MYSQL_STMT* sessionStmt = mysql_stmt_init(conn);
    const char* sessionSql = "INSERT INTO sessions (user_id, session_token, login_time, last_active, ip_address) "
        "VALUES (?, ?, NOW(), NOW(), ?)";
    if (!sessionStmt || mysql_stmt_prepare(sessionStmt, sessionSql, std::strlen(sessionSql)) != 0) {
        renderRegisterForm(email, "Internal server error. Please try again later.");
        closeDBConnection(conn);
        return 0;
    }

    MYSQL_BIND sessionParam[3];
    std::memset(sessionParam, 0, sizeof(sessionParam));
    sessionParam[0].buffer_type = MYSQL_TYPE_LONGLONG;
    sessionParam[0].buffer = &newUserId;
    sessionParam[0].is_unsigned = 1;

    sessionParam[1].buffer_type = MYSQL_TYPE_STRING;
    sessionParam[1].buffer = (char*)sessionToken.c_str();
    sessionParam[1].buffer_length = sessionToken.size();

    sessionParam[2].buffer_type = MYSQL_TYPE_STRING;
    sessionParam[2].buffer = (char*)ipAddress.c_str();
    sessionParam[2].buffer_length = ipAddress.size();

    if (mysql_stmt_bind_param(sessionStmt, sessionParam) != 0 || mysql_stmt_execute(sessionStmt) != 0) {
        renderRegisterForm(email, "Internal server error. Please try again later.");
        mysql_stmt_close(sessionStmt);
        closeDBConnection(conn);
        return 0;
    }

    mysql_stmt_close(sessionStmt);
    closeDBConnection(conn);

    renderRegisterSuccess(email, sessionToken);
    return 0;
}
