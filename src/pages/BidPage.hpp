// pages/BidPage.hpp
#pragma once

#ifndef PAGES_BIDPAGE_HPP
#define PAGES_BIDPAGE_HPP

#include "core/Page.hpp"
#include <string>
#include <vector>

// ------------------------------------------------------------------
// BidPage
// - Pure-UI item detail + place-bid template
// - Uses shared header/footer (Page::printHead / printTail)
// - Backend hookup points are documented in BidPage.cpp
// ------------------------------------------------------------------
class BidPage : public Page {
public:
    BidPage(Database& db, Session& session);

    // Renders the bid page UI
    void handleGet() override;

    // Handles a POSTed bid amount (stub UI only; no DB writes yet)
    void handlePost() override;

private:
    // Special struct for handling bid items and their id/names
    struct ItemOption{
        long id;
        std::string title;
    };
    
    // For active items
    std::vector<ItemOption> fetchActiveItemsExcludingSeller(long excludeSellerId);

    // Load seller_id, start_price, current_max_bid, and whether auction is active
    bool loadItemAndState(long itemId,
        long& sellerId,
        double& startPrice,
        double& currentMaxBid,
        bool& isActive);

    // Shared renderer used by GET and POST (preserves entered values / flash)
    void renderForm(const std::vector<ItemOption>& items,
        const std::string& flashError = "",
        const std::string& flashSuccess = "",
        long selectedItemId = 0,
        const std::string& enteredAmount = "");
};
};

#endif // PAGES_BIDPAGE_HPP
