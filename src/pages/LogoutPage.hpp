// pages/LogoutPage.hpp
#pragma once

#include "core/Page.hpp"
#include "core/Database.hpp"
#include "core/Session.hpp"

// -------------------------------------------------------------
// LogoutPage
// -------------------------------------------------------------
// Ends the user's session and displays confirmation.
// -------------------------------------------------------------
class LogoutPage : public Page {
public:
    LogoutPage(Database& db, Session& session);

protected:
    void handleGet() override;   // logout is always GET-driven
};
