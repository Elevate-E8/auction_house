// pages/BrowsePage.hpp
#ifndef TEA_PAGES_BROWSE_PAGE_HPP
#define TEA_PAGES_BROWSE_PAGE_HPP

#include "core/Page.hpp"

// -----------------------------------------------------------------------------
// BrowsePage
// Visual-only templated page for listing auctions. No DB wiring here.
// Back-end can populate items via handleGet() once connected to DB.
// -----------------------------------------------------------------------------
class BrowsePage : public Page {
public:
    explicit BrowsePage(Database& db, Session& session);

    // GET: render the browse UI with pagination controls.
    // Note: This is a static template right now; see BrowsePage.cpp
    // for the "INSERT BACKEND HERE" markers where items should be injected.
    void handleGet() override;

    // No POST handling for now.
    void handlePost() override {}  // keep to satisfy virtual in base if desired
};

#endif // TEA_PAGES_BROWSE_PAGE_HPP
