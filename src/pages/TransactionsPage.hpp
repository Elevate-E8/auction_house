// pages/TransactionsPage.hpp
#pragma once

#include "core/Page.hpp"
#include "core/Database.hpp"
#include "core/Session.hpp"

// -------------------------------------------------------------
// TransactionsPage
// -------------------------------------------------------------
// Displays all user-related auction activity:
//   - Current bids
//   - Selling listings
//   - Purchases
//   - Lost auctions
// Requires login; redirects to login page if not authenticated.
// -------------------------------------------------------------
class TransactionsPage : public Page {
public:
    TransactionsPage(Database& db, Session& session);

protected:
    void handleGet() override;
};
