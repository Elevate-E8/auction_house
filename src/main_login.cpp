
// main_login.cpp
#include "core/Database.hpp"
#include "core/Session.hpp"
#include "pages/LoginPage.hpp"
#include <iostream>

int main() {
    try {
        Database db;
        Session session(db);
        LoginPage page(db, session);
        return page.run();
    }
    catch (const std::exception& e) {
        std::cout << "Content-type: text/plain\n\n";
        std::cout << "Internal error: " << e.what() << "\n";
        return 1;
    }
}
