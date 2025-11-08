// pages/SellPage.hpp
#pragma once

#include "core/Page.hpp"
#include "core/Database.hpp"
#include "core/Session.hpp"

// -------------------------------------------------------------
// SellPage
// -------------------------------------------------------------
// Front-end page that renders a form to list an item for auction.
// Fields: description, starting price, start datetime.
// Duration is fixed at 168 hours (7 days) from the start time.
// -------------------------------------------------------------
class SellPage : public Page {
public:
    SellPage(Database& db, Session& session);

protected:
    void handleGet() override; // render the Sell form (no backend logic here)
};
