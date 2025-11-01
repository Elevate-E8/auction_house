// main_transactions.cpp
#include "core/Database.hpp"
#include "core/Session.hpp"
#include "pages/TransactionsPage.hpp"
#include <iostream>

int main() {
    try {
        Database db;
        Session session(db);
        TransactionsPage page(db, session);
        return page.run();
    }
    catch (const std::exception& e) {
        std::cout << "Content-type: text/plain\n\n";
        std::cout << "Internal error: " << e.what() << "\n";
        return 1;
    }
}
