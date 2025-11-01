#pragma once
#include <mysql/mysql.h>
#include <string>

class Database {
public:
    explicit Database(const std::string& host = "localhost",
        const std::string& user = "cs370_section2_elevate",
        const std::string& pass = "etavele_004",
        const std::string& db = "cs370_section2_elevate",
        unsigned int port = 0);

    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool execute(const std::string& sql,
        MYSQL_BIND* params = nullptr,
        unsigned int count = 0);

    MYSQL* connection() const noexcept;

private:
    MYSQL* conn_;
};
