// pages/IndexPage.hpp
#pragma once

#include "core/Page.hpp"
#include "core/Database.hpp"
#include "core/Session.hpp"

// -------------------------------------------------------------
// IndexPage
// -------------------------------------------------------------
// Homepage with navigation and user session awareness.
// Shows different links if logged in or logged out.
// -------------------------------------------------------------
class IndexPage : public Page {
public:
    IndexPage(Database& db, Session& session);

protected:
    void handleGet() override;
};
