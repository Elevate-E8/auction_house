// core/Page.hpp
#pragma once

#include <string>
#include <map>
#include <mysql/mysql.h>
#include "core/Database.hpp"
#include "core/Session.hpp"

class Page {
protected:
    Database& db_;
    Session& session_;
    std::map<std::string, std::string> postData_;

public:
    Page(Database& db, Session& session);
    virtual ~Page() = default;

    int run();

protected:
    virtual void handleGet() = 0;
    virtual void handlePost() {}

    void sendHTMLHeader() const;
    void printHead(const std::string& title, const std::string& mode = "") const;
    void printTail(const std::string& mode = "") const;

    void parsePost();
};
