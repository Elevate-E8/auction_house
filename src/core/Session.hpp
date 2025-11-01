// core/Session.hpp
#pragma once
#include <string>
#include "core/Database.hpp"

class Session {
public:
    explicit Session(Database& db);

    bool isLoggedIn() const noexcept { return loggedIn_; }
    const std::string& userEmail() const noexcept { return email_; }
    long userId() const noexcept { return userId_; }

    void create(long uid, const std::string& token, const std::string& ip);
    void destroy();
    bool validate();

private:
    Database& db_;
    std::string token_;
    std::string email_;
    long userId_;
    bool loggedIn_;

    std::string readCookieToken() const;
};
