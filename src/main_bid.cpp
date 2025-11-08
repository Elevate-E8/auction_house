// src/main_bid.cpp
// -------------------------------------------------------------
// Entry point for Bid page (CGI: bid.cgi)
//
// This file intentionally stays minimal so backend devs can
// later add routing/lookup logic (e.g., reading ?item_id=...,
// loading item details, auth checks, etc.).
//
// TODO ideas for backend:
// 1) Parse QUERY_STRING to get `item_id` and load item from DB.
// 2) Pass loaded item data to BidPage (via setters or extend
//    BidPage ctor to accept an item DTO).
// 3) Enforce login (redirect to login.cgi if not logged in).
// 4) Add graceful error handling for missing/invalid items.
// -------------------------------------------------------------

#include "core/Database.hpp"
#include "core/Session.hpp"
#include "pages/BidPage.hpp"

#include <iostream>
#include <exception>

int main() {
    try {
        // Initialize shared services
        Database db;          // connects lazily or in ctor depending on your impl
        Session session(db);  // session backed by DB

        // Render the page (handles GET/POST internally)
        BidPage page(db, session);
        return page.run();
    }
    catch (const std::exception& e) {
        // Last-resort error response (avoid leaking details in production)
        std::cout << "Content-Type: text/html\r\n\r\n";
        std::cout << "<!doctype html><html lang='en'><head><meta charset='utf-8'>"
                     "<title>Server Error</title></head><body>"
                     "<h1>Server Error</h1>"
                     "<p>Something went wrong while loading the Bid page.</p>"
                     "</body></html>";
        return 1;
    }
}
