// core/Page.cpp
#include "core/Page.hpp"
#include "utils/utils.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>

Page::Page(Database& db, Session& session)
    : db_(db), session_(session) {
}

// -------------------------------------------------------------
// Entry point dispatcher
// -------------------------------------------------------------
int Page::run() {
    const char* method = std::getenv("REQUEST_METHOD");
    if (method && std::string(method) == "POST") {
        parsePost();
        handlePost();
    } else {
        handleGet();
    }
    return 0;
}

// -------------------------------------------------------------
// Sends required CGI header
// -------------------------------------------------------------
void Page::sendHTMLHeader() const {
    std::cout << "Content-Type: text/html\r\n\r\n";
}

// -------------------------------------------------------------
// Shared HTML head and navigation
// mode: "auth" for login/register/logout (centered card)
//       anything else for content pages (container layout)
// -------------------------------------------------------------
void Page::printHead(const std::string& title, const std::string& mode) const {
    bool isLoggedIn = session_.isLoggedIn();
    std::string email = isLoggedIn ? session_.userEmail() : "";

    std::cout
        << "<!doctype html>\n"
        << "<html lang='en'>\n"
        << "<head>\n"
        << "  <meta charset='utf-8'>\n"
        << "  <meta name='viewport' content='width=device-width, initial-scale=1'>\n"
        << "  <meta http-equiv='refresh' content='305'>\n"
        << "  <title>" << htmlEscape(title) << "</title>\n"
        << "  <link rel='stylesheet' href='../css/main.css'>\n"
        << "</head>\n"
        << "<body class='" << mode << "'>\n";

    // Shared header/nav
    std::cout
        << "<header>\n"
        << "  <div class='container'>\n"
        << "    <div class='nav'>\n"
        << "      <div class='brand'>\n"
        << "        <span class='logo'></span>\n"
        << "        <h1 style='margin:0'>"
        << "          <a class='brand-link' href='index.cgi'>Team Elevate Auctions</a>"
        << "        </h1>\n"
        << "      </div>\n"
        << "      <nav class='links'>\n";

    // Only show Bid/Sell when logged in
    if (isLoggedIn) {
        std::cout
            << "        <a href='browse.cgi'>Browse Auctions</a>\n"
            << "        <a href='sell.cgi'>Sell</a>\n"
            << "        <a href='transactions.cgi'>My Transactions</a>\n"
            << "        <a href='logout.cgi'>Logout</a>\n";
    } else {
        std::cout
            << "        <a href='login.cgi'>Login</a>\n"
            << "        <a href='register.cgi'>Register</a>\n";
    }

    std::cout
        << "      </nav>\n"
        << "    </div>\n"
        << "  </div>\n"
        << "</header>\n";

    // Body open
    if (mode == "auth") {
        std::cout << "<main>\n";
    } else {
        std::cout << "<main><div class='container'>\n";
    }
}

// -------------------------------------------------------------
// Shared footer
// -------------------------------------------------------------
void Page::printTail(const std::string& mode) const {
    if (mode == "auth") {
        std::cout
            << "<footer>&copy; 2025 Team Elevate. All rights reserved.</footer>\n"
            << "</main>\n";
    } else {
        std::cout
            << "<footer>&copy; 2025 Team Elevate. All rights reserved.</footer>\n"
            << "</div></main>\n";
    }
    std::cout << "</body></html>\n";
}

// -------------------------------------------------------------
// Parse POST
// -------------------------------------------------------------
void Page::parsePost() {
    postData_ = parsePostData();
}
