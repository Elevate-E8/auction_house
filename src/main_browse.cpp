// main_browse.cpp
#include "core/Database.hpp"
#include "core/Session.hpp"
#include "pages/BrowsePage.hpp"
#include "utils/utils.hpp"   // for htmlEscape on error path
#include <iostream>
#include <exception>

int main() {
    try {
        // If your Database needs DSN/user/pass, your Database() ctor likely
        // already reads them from a config/env. Otherwise, adjust here.
        Database db;
        Session session(db);

        BrowsePage page(db, session);
        return page.run();
    } catch (const std::exception& e) {
        // CGI error fallback: always emit the header before any HTML
        std::cout << "Content-Type: text/html\r\n\r\n";
        std::cout
            << "<!doctype html><html lang='en'><head>"
            << "<meta charset='utf-8'><title>Server Error</title>"
            << "<link rel='stylesheet' href='../css/main.css'>"
            << "</head><body class='auth'>"
            << "<main><section class='card'>"
            << "<h1>Something went wrong</h1>"
            << "<div class='error'>"
            << htmlEscape(e.what())
            << "</div>"
            << "<p class='helper'>Please try again later.</p>"
            << "</section></main>"
            << "</body></html>";
        return 1;
    }
}
