// pages/LogoutPage.cpp
#include "pages/LogoutPage.hpp"
#include "utils/utils.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>

LogoutPage::LogoutPage(Database& db, Session& session)
    : Page(db, session) {
}

// -------------------------------------------------------------
// GET — perform logout and show confirmation
// -------------------------------------------------------------
void LogoutPage::handleGet() {
    // Retrieve session token from cookies
    const char* cookieEnv = std::getenv("HTTP_COOKIE");
    std::string sessionToken;
    if (cookieEnv) {
        sessionToken = getCookieValue(std::string(cookieEnv), "session_token");
    }

    // Delete session from database if token exists
    if (!sessionToken.empty()) {
        MYSQL* conn = db_.connection();
        if (conn) {
            const char* sql = "DELETE FROM sessions WHERE session_token=?";
            MYSQL_STMT* stmt = mysql_stmt_init(conn);
            if (stmt && mysql_stmt_prepare(stmt, sql, std::strlen(sql)) == 0) {
                MYSQL_BIND param{};
                memset(&param, 0, sizeof(param));
                param.buffer_type = MYSQL_TYPE_STRING;
                param.buffer = (char*)sessionToken.c_str();
                param.buffer_length = sessionToken.size();
                mysql_stmt_bind_param(stmt, &param);
                mysql_stmt_execute(stmt);
                mysql_stmt_close(stmt);
            }
        }
    }

    // Clear the cookie
    std::cout << "Content-Type: text/html\r\n";
    std::cout << "Set-Cookie: session_token=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Max-Age=0\r\n";
    std::cout << "Cache-Control: no-store, no-cache, must-revalidate\r\n";
    std::cout << "Pragma: no-cache\r\n";
    std::cout << "\r\n"; // end headers

    // Ensure the nav renders as logged-out for THIS response
    setenv("HTTP_COOKIE", "", 1);

    // Output HTML
    printHead("Logged Out · Team Elevate", "auth");
    std::cout
        << "    <section class='card' role='status' aria-live='polite'>\n"
        << "      <h1>✓ Signed out</h1>\n"
        << "      <p class='muted'>Your session has ended. We’re taking you back to the homepage…</p>\n"
        << "      <meta http-equiv='refresh' content='2;url=index.cgi'>\n"
        << "      <a class='btn primary' href='index.cgi'>Go to Home</a>\n"
        << "    </section>\n";
    printTail("auth");
}
