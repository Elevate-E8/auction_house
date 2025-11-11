#include "pages/LoginPage.hpp"
#include "utils/utils.hpp"
#include <iostream>
#include <cstring>

// -------------------------------------------------------------
// Constructor
// -------------------------------------------------------------
LoginPage::LoginPage(Database& db, Session& session)
    : Page(db, session) {
}

// -------------------------------------------------------------
// GET — Show the login form
// -------------------------------------------------------------
void LoginPage::handleGet() {
    sendHTMLHeader();
    printHead("Login · Team Elevate", "auth");

    std::cout
        << "    <section class='card'>\n"
        << "      <h2>Welcome back</h2>\n"
        << "      <p class='muted'>Log in to continue bidding.</p>\n"
        << "      <form method='post' action='login.cgi'>\n"
        << "        <div>\n"
        << "          <label for='email'>Email</label>\n"
        << "          <input id='email' name='email' type='email' maxlength='100' required autocomplete='email'>\n"
        << "        </div>\n"
        << "        <div>\n"
        << "          <label for='password'>Password</label>\n"
        << "          <input id='password' name='password' type='password' minlength='8' required autocomplete='current-password'>\n"
        << "        </div>\n"
        << "        <button class='btn primary' type='submit'>Log In</button>\n"
        << "      </form>\n"
        << "      <div class='top-gap helper'>Don't have an account? <a href='register.cgi'>Create one</a>.</div>\n"
        << "    </section>\n";

    printTail("auth");
}

// -------------------------------------------------------------
// POST — Handle login submission
// -------------------------------------------------------------
void LoginPage::handlePost() {
    std::string email = postData_["email"];
    std::string password = postData_["password"];

    // Minimal helper to re-render the same form with an error message
    auto showFormWithError = [&](const std::string& msg) {
        sendHTMLHeader();
        printHead("Login · Team Elevate", "auth");
        if (!msg.empty()) {
            std::cout << "    <div class='error' role='alert'>" << htmlEscape(msg) << "</div>\n";
        }
        std::cout
            << "    <section class='card'>\n"
            << "      <h2>Welcome back</h2>\n"
            << "      <p class='muted'>Log in to continue bidding.</p>\n"
            << "      <form method='post' action='login.cgi'>\n"
            << "        <div>\n"
            << "          <label for='email'>Email</label>\n"
            << "          <input id='email' name='email' type='email' maxlength='100' required "
            << "autocomplete='email' value='" << htmlEscape(email) << "'>\n"
            << "        </div>\n"
            << "        <div>\n"
            << "          <label for='password'>Password</label>\n"
            << "          <input id='password' name='password' type='password' minlength='8' required "
            << "autocomplete='current-password' autofocus>\n"
            << "        </div>\n"
            << "        <button class='btn primary' type='submit'>Log In</button>\n"
            << "      </form>\n"
            << "      <div class='top-gap helper'>Don't have an account? <a href='register.cgi'>Create one</a>.</div>\n"
            << "    </section>\n";
        printTail("auth");
    };

    if (email.empty() || password.empty()) {
        showFormWithError("Email and password are required.");
        return;
    }

    MYSQL* conn = db_.connection();
    if (!conn) {
        showFormWithError("Database connection failed. Please try again later.");
        return;
    }

    std::string hashedPassword = hashPassword(password);
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    const char* sql = "SELECT user_id FROM users WHERE user_email=? AND password_hash=? LIMIT 1";

    if (!stmt || mysql_stmt_prepare(stmt, sql, std::strlen(sql)) != 0) {
        if (stmt) mysql_stmt_close(stmt);
        showFormWithError("Internal server error.");
        return;
    }

    MYSQL_BIND params[2];
    memset(params, 0, sizeof(params));
    params[0].buffer_type   = MYSQL_TYPE_STRING;
    params[0].buffer        = (char*)email.c_str();
    params[0].buffer_length = email.size();
    params[1].buffer_type   = MYSQL_TYPE_STRING;
    params[1].buffer        = (char*)hashedPassword.c_str();
    params[1].buffer_length = hashedPassword.size();

    if (mysql_stmt_bind_param(stmt, params) != 0 || mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        showFormWithError("Internal server error.");
        return;
    }

    MYSQL_BIND result{};
    long long userId = 0;
    result.buffer_type = MYSQL_TYPE_LONGLONG;
    result.buffer      = &userId;
    result.is_unsigned = 1;
    mysql_stmt_bind_result(stmt, &result);
    mysql_stmt_store_result(stmt);

    int fetchStatus = mysql_stmt_fetch(stmt);
    mysql_stmt_close(stmt);

    if (fetchStatus != 0) {
        // Wrong email/password -> show the same page + error, keep email filled
        showFormWithError("Invalid email or password.");
        return;
    }

    // Create session entry in DB
    std::string sessionToken = generateSessionToken();
    const char* remoteAddr = std::getenv("REMOTE_ADDR");
    std::string ipAddress = remoteAddr ? std::string(remoteAddr) : "unknown";
    session_.create(userId, sessionToken, ipAddress);

    // Cookie header must come before any HTML
    std::cout << "Content-Type: text/html\r\n";
    std::cout << "Set-Cookie: session_token=" << sessionToken
              << "; Path=/; HttpOnly; SameSite=Lax\r\n\r\n";

    // Success page
    printHead("Login Successful", "auth");
    std::cout
        << "    <section class='card' role='status' aria-live='polite'>\n"
        << "      <h2>✓ Login successful</h2>\n"
        << "      <div class='success'>Signed in as <strong>" << htmlEscape(email) << "</strong>.</div>\n"
        << "      <p class='muted'>Redirecting to the homepage…</p>\n"
        << "      <meta http-equiv='refresh' content='2;url=index.cgi'>\n"
        << "      <a class='btn primary' href='index.cgi'>Go to Home</a>\n"
        << "    </section>\n";
    printTail("auth");
}
