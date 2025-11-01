// pages/RegisterPage.hpp
#pragma once

#include "core/Page.hpp"
#include "core/Database.hpp"
#include "core/Session.hpp"

// -------------------------------------------------------------
// RegisterPage
// -------------------------------------------------------------
// Displays the registration form (GET)
// Handles new account creation and session start (POST)
// -------------------------------------------------------------

class RegisterPage : public Page {
public:
    RegisterPage(Database& db, Session& session);

protected:
    void handleGet() override;
    void handlePost() override;
};
