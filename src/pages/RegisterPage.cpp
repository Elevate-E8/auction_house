// pages/RegisterPage.cpp
#include "pages/RegisterPage.hpp"
#include "utils/utils.hpp"
#include <iostream>
#include <cstring>

RegisterPage::RegisterPage(Database& db, Session& session)
    : Page(db, session) {
}

// -------------------------------------------------------------
// GET — Show the registration form
// -------------------------------------------------------------
void RegisterPage::handleGet() {
    sendHTMLHeader();
    printHead("Register · Team Elevate", "auth");

    std::cout
        << "    <section class='card'>\n"
        << "      <h1>Create your account</h1>\n"
        << "      <p class='muted'>Join Team Elevate to start bidding and tracking your favorites.</p>\n"
        << "      <form method='post' action='register.cgi'>\n"
        << "        <div>\n"
        << "          <label for='email'>Email</label>\n"
        << "          <input id='email' name='email' type='email' maxlength='100' required autocomplete='email'>\n"
        << "        </div>\n"
        << "        <div>\n"
        << "          <label for='password'>Password</label>\n"
        << "          <input id='password' name='password' type='password' minlength='8' required autocomplete='new-password'>\n"
        << "          <div class='helper'>Use at least 8 characters.</div>\n"
        << "        </div>\n"
        << "        <div>\n"
        << "          <label for='confirm'>Confirm Password</label>\n"
        << "          <input id='confirm' name='confirm' type='password' minlength='8' required autocomplete='new-password'>\n"
        << "        </div>\n"
        << "        <button class='btn primary' type='submit'>Create account</button>\n"
        << "      </form>\n"
        << "      <div class='top-gap helper'>Already have an account? <a href='login.cgi'>Log in</a>.</div>\n"
        << "    </section>\n";

    printTail("auth");
}

// -------------------------------------------------------------
// POST — Handle registration submission
// -------------------------------------------------------------
void RegisterPage::handlePost() {
    std::string email = postData_["email"];
    std::string password = postData_["password"];
    std::string confirm = postData_["confirm"];

    // Validate basic inputs
    if (email.empty() || password.empty() || confirm.empty()) {
        sendHTMLHeader();
        printHead("Register · Team Elevate", "auth");
        std::cout << "<div class='error'>All fields are required.</div>\n";
        printTail("auth");
        return;
    }
    if (!isValidEmail(email)) {
        sendHTMLHeader();
        printHead("Register · Team Elevate", "auth");
        std::cout << "<div class='error'>Please enter a valid email address.</div>\n";
        printTail("auth");
        return;
    }
    if (password.size() < 8) {
        sendHTMLHeader();
        printHead("Register · Team Elevate", "auth");
        std::cout << "<div class='error'>Password must be at least 8 characters.</div>\n";
        printTail("auth");
        return;
    }
    if (password != confirm) {
        sendHTMLHeader();
        printHead("Register · Team Elevate", "auth");
        std::cout << "<div class='error'>Passwords do not match.</div>\n";
        printTail("auth");
        return;
    }

    MYSQL* conn = db_.connection();
    if (!conn) {
        sendHTMLHeader();
        printHead("Register · Team Elevate", "auth");
        std::cout << "<div class='error'>Internal server error. Please try again later.</div>\n";
        printTail("auth");
        return;
    }

    // Check if user already exists
    const char* checkSql = "SELECT user_id FROM users WHERE user_email=? LIMIT 1";
    MYSQL_STMT* checkStmt = mysql_stmt_init(conn);
    if (!checkStmt || mysql_stmt_prepare(checkStmt, checkSql, std::strlen(checkSql)) != 0) {
        if (checkStmt) mysql_stmt_close(checkStmt);
        sendHTMLHeader();
        printHead("Register · Team Elevate", "auth");
        std::cout << "<div class='error'>Internal server error.</div>\n";
        printTail("auth");
        return;
    }

    MYSQL_BIND checkParam{};
    memset(&checkParam, 0, sizeof(checkParam));
    checkParam.buffer_type = MYSQL_TYPE_STRING;
    checkParam.buffer = (char*)email.c_str();
    checkParam.buffer_length = email.size();

    mysql_stmt_bind_param(checkStmt, &checkParam);
    mysql_stmt_execute(checkStmt);
    MYSQL_BIND checkResult{};
    my_ulonglong dummyId = 0;
    checkResult.buffer_type = MYSQL_TYPE_LONGLONG;
    checkResult.buffer = &dummyId;
    checkResult.is_unsigned = 1;
    mysql_stmt_bind_result(checkStmt, &checkResult);
    mysql_stmt_store_result(checkStmt);
    bool exists = (mysql_stmt_fetch(checkStmt) == 0);
    mysql_stmt_close(checkStmt);

    if (exists) {
        sendHTMLHeader();
        printHead("Register · Team Elevate", "auth");
        std::cout << "<div class='error'>This email is already registered. Try logging in instead.</div>\n";
        printTail("auth");
        return;
    }

    // Insert new user
    std::string hashedPassword = hashPassword(password);
    const char* insertSql = "INSERT INTO users (user_email, password_hash, joindate) VALUES (?, ?, NOW())";
    MYSQL_STMT* insertStmt = mysql_stmt_init(conn);
    if (!insertStmt || mysql_stmt_prepare(insertStmt, insertSql, std::strlen(insertSql)) != 0) {
        if (insertStmt) mysql_stmt_close(insertStmt);
        sendHTMLHeader();
        printHead("Register · Team Elevate", "auth");
        std::cout << "<div class='error'>Internal server error.</div>\n";
        printTail("auth");
        return;
    }

    MYSQL_BIND insertParams[2];
    memset(insertParams, 0, sizeof(insertParams));
    insertParams[0].buffer_type = MYSQL_TYPE_STRING;
    insertParams[0].buffer = (char*)email.c_str();
    insertParams[0].buffer_length = email.size();
    insertParams[1].buffer_type = MYSQL_TYPE_STRING;
    insertParams[1].buffer = (char*)hashedPassword.c_str();
    insertParams[1].buffer_length = hashedPassword.size();

    mysql_stmt_bind_param(insertStmt, insertParams);
    if (mysql_stmt_execute(insertStmt) != 0) {
        mysql_stmt_close(insertStmt);
        sendHTMLHeader();
        printHead("Register · Team Elevate", "auth");
        std::cout << "<div class='error'>Could not create account.</div>\n";
        printTail("auth");
        return;
    }

    unsigned long long newUserId = mysql_insert_id(conn);
    mysql_stmt_close(insertStmt);

    // Create new session
    std::string sessionToken = generateSessionToken();
    const char* remoteAddr = std::getenv("REMOTE_ADDR");
    std::string ipAddress = remoteAddr ? std::string(remoteAddr) : "unknown";
    session_.create(newUserId, sessionToken, ipAddress);

    // ---------------------------------------------------------
    // Correct header order for cookies
    // ---------------------------------------------------------
    std::cout << "Content-Type: text/html\r\n";
    std::cout << "Set-Cookie: session_token=" << sessionToken
        << "; Path=/; HttpOnly; SameSite=Lax\r\n\r\n";

    // ---------------------------------------------------------
    // Success HTML
    // ---------------------------------------------------------
    printHead("Registration Successful", "auth");
    std::cout
        << "    <section class='card'>\n"
        << "      <h1>✓ Registration Successful</h1>\n"
        << "      <div class='success'>Welcome to Team Elevate, " << htmlEscape(email) << ".</div>\n"
        << "      <p class='muted'>You’ll be redirected to the home page momentarily.</p>\n"
        << "      <meta http-equiv='refresh' content='3;url=index.cgi'>\n"
        << "      <div class='top-gap'><a class='btn primary' href='index.cgi'>Go now</a></div>\n"
        << "    </section>\n";
    printTail("auth");
}
