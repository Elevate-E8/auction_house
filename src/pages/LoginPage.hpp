// pages/LoginPage.hpp
#pragma once

#include "core/Page.hpp"
#include "core/Database.hpp"
#include "core/Session.hpp"

// -------------------------------------------------------------
// LoginPage
// -------------------------------------------------------------
// Represents the login page in the class-based CGI architecture.
// Handles both GET (form display) and POST (form submission).
// Maintains all current visual formatting and layout.
// -------------------------------------------------------------

class LoginPage : public Page {
public:
    // Constructor
    LoginPage(Database& db, Session& session);

protected:
    // Called for GET requests
    void handleGet() override;

    // Called for POST requests
    void handlePost() override;
};
