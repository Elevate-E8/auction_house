// pages/BidPage.hpp
#ifndef PAGES_BIDPAGE_HPP
#define PAGES_BIDPAGE_HPP

#include "core/Page.hpp"
#include <string>

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
    // Shared renderer used by both GET and POST so the layout stays in sync.
    void renderBidTemplate(const std::string& itemName,
                           const std::string& condition,
                           const std::string& description,
                           const std::string& startingPrice,
                           const std::string& currentBid,
                           const std::string& itemId,
                           const std::string& endsAt,
                           const std::string& flashHtml);
};

#endif // PAGES_BIDPAGE_HPP
