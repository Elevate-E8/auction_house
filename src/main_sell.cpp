// main_sell.cpp
#include "core/Database.hpp"
#include "core/Session.hpp"
#include "pages/SellPage.hpp"
#include <iostream>

int main() {
    try {
        Database db;
        Session session(db);
        SellPage page(db, session);
        return page.run();
    } catch (const std::exception& e) {
        std::cout << "Content-type: text/plain\n\n";
        std::cout << "Internal error: " << e.what() << "\n";
        return 1;
    }
}
