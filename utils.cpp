#include "utils.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <random>
#include <openssl/sha.h>
#include <mysql/mysql.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>

// ----------------- Extract Cookie -----------------
std::string getCookieValue(const std::string& cookies, const std::string& name) {
    size_t start = cookies.find(name + "=");
    if (start == std::string::npos)
        return "";
    start += name.length() + 1;
    size_t end = cookies.find(";", start);
    if (end == std::string::npos)
        end = cookies.length();
    return cookies.substr(start, end - start);
}

// ----------------- Create DB Connection -----------------
MYSQL* createDBConnection() {
    MYSQL* conn = mysql_init(nullptr);
    if (!conn) {
        std::cerr << "MySQL initialization failed.\n";
        return nullptr;
    }

    if (!mysql_real_connect(conn,
        "localhost",
        "cs370_section2_elevate",
        "etavele_004",
        "cs370_section2_elevate",
        0, nullptr, 0)) {
        std::cerr << "MySQL connection failed: " << mysql_error(conn) << "\n";
        mysql_close(conn);
        return nullptr;
    }
    return conn;
}

// ----------------- Close DB Connection -----------------
void closeDBConnection(MYSQL* conn) {
    if (conn) mysql_close(conn);
}

// ----------------- HTML Escape Helper -----------------
std::string htmlEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '&':  out += "&amp;";  break;
        case '<':  out += "&lt;";   break;
        case '>':  out += "&gt;";   break;
        case '"':  out += "&quot;"; break;
        case '\'': out += "&#x27;"; break;
        default:   out += c;        break;
        }
    }
    return out;
}

// ----------------- URL Decode Helper -----------------
std::string urlDecode(const std::string& str) {
    std::string result;
    for (std::size_t i = 0; i < str.length(); i++) {
        if (str[i] == '+') result += ' ';
        else if (str[i] == '%' && i + 2 < str.length()) {
            unsigned int value;
            std::sscanf(str.substr(i + 1, 2).c_str(), "%x", &value);
            result += static_cast<char>(value);
            i += 2;
        }
        else result += str[i];
    }
    return result;
}

// ----------------- Parse POST Data -----------------
std::map<std::string, std::string> parsePostData() {
    std::map<std::string, std::string> data;
    char* contentLength = std::getenv("CONTENT_LENGTH");
    if (contentLength) {
        int length = std::atoi(contentLength);
        if (length > 0) {
            std::string postData;
            postData.resize(length);
            std::cin.read(&postData[0], length);

            std::stringstream ss(postData);
            std::string pair;
            while (std::getline(ss, pair, '&')) {
                size_t pos = pair.find('=');
                if (pos != std::string::npos) {
                    std::string key = pair.substr(0, pos);
                    std::string value = pair.substr(pos + 1);
                    data[key] = urlDecode(value);
                }
            }
        }
    }
    return data;
}

// ----------------- SHA-256 Password Hash -----------------
std::string hashPassword(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(password.c_str()),
        password.length(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    return ss.str();
}

// ----------------- Session Token Generator -----------------
std::string generateSessionToken() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::stringstream ss;
    for (int i = 0; i < 32; i++)
        ss << std::hex << dis(gen);
    return ss.str();
}

// ----------------- Email Validator -----------------
bool isValidEmail(const std::string& email) {
    std::size_t atPos = email.find('@');
    std::size_t dotPos = email.find('.', atPos);
    return (atPos != std::string::npos && dotPos != std::string::npos &&
        atPos > 0 && dotPos > atPos + 1 && dotPos < email.length() - 1);
}


// --------------Auto-detecting header / footer output--------------


void sendHTMLHeader() {
    std::cout << "Content-Type: text/html\n\n";
}

void printHead(const std::string& title, const std::string& layout) {
    std::string email;
    bool isLoggedIn = isUserLoggedIn(email);

    std::cout << "<!doctype html>\n<html lang='en'>\n<head>\n"
        << "  <meta charset='utf-8'>\n"
        << "  <meta name='viewport' content='width=device-width, initial-scale=1'>\n"
        << "  <meta http-equiv='refresh' content='305'>\n"
        << "  <title>" << title << "</title>\n"
        << "  <style>\n"
        << "    :root {\n"
        << "      --ink:#0f172a;--muted:#475569;--bg:#f6f7fb;--card:#ffffff;\n"
        << "      --brand:#4f46e5;--brand-2:#22c55e;--border:#e5e7eb;\n"
        << "      --shadow:0 6px 18px rgba(2,6,23,.08);--radius:14px;\n"
        << "    }\n"
        << "    *, *::before, *::after { box-sizing: border-box; }\n"
        << "    html, body { height: 100%; }\n"
        << "    body {\n"
        << "      margin:0; padding:0;\n"
        << "      font-family: system-ui, -apple-system, Segoe UI, Roboto, Arial, sans-serif;\n"
        << "      line-height:1.6; color:var(--ink); background:var(--bg);\n"
        << "      -webkit-font-smoothing: antialiased; -moz-osx-font-smoothing: grayscale;\n"
        << "    }\n"
        << "    header { background:linear-gradient(135deg,#111827,#1f2937 50%,#111827); color:#fff; }\n"
        << "    .container { max-width:1100px; margin-inline:auto; padding-inline:20px; }\n"
        << "    .nav { display:flex; align-items:center; justify-content:space-between; padding:18px 0; }\n"
        << "    .brand { display:flex; align-items:center; gap:12px; }\n"
        << "    .logo { width:36px; height:36px; border-radius:10px; background:linear-gradient(135deg,var(--brand),#7c3aed); }\n"
        << "    .brand h1 { margin:0; font-size:18px; letter-spacing:.3px; color:#fff; }\n"
        << "    .links { display:flex; gap:14px; align-items:center; flex-wrap:wrap; }\n"
        << "    .links a { color:#e5e7eb; text-decoration:none; font-weight:600; padding:6px 8px; border-radius:8px; }\n"
        << "    .links a:hover { color:#fff; background:rgba(255,255,255,.08); }\n"
        << "    main { padding:22px 0; }\n"
        << "    .card { background:var(--card); border-radius:var(--radius); box-shadow:var(--shadow); padding:28px; border:1px solid var(--border); }\n"
        << "    /* Layout modifiers */\n"
        << "    .auth main { display:grid; place-items:center; padding:28px 0; }\n"
        << "    .auth .card { max-width:480px; width:100%; }\n"
        << "    .content main { padding:22px 0; }\n"
        << "    h1 { margin-top:0; font-size:26px; color:var(--ink); }\n"
        << "    p.muted { color:var(--muted); margin:0 0 20px; }\n"
        << "    form { display:flex; flex-direction:column; gap:14px; }\n"
        << "    label { font-weight:700; color:#374151; font-size:14px; }\n"
        << "    input { width:100%; padding:12px 14px; border:1px solid #e5e7eb; border-radius:12px; font-size:15px; background:#fff; }\n"
        << "    input:focus { outline:2px solid var(--brand); border-color:transparent; }\n"
        << "    .btn { display:inline-block; background:var(--brand); color:#fff; border:none; border-radius:12px; padding:12px 16px; font-weight:700; cursor:pointer; text-decoration:none; }\n"
        << "    .btn:hover { filter:brightness(1.05); }\n"
        << "    .error { background:#fef2f2; border:1px solid #fecaca; color:#991b1b; padding:12px 14px; border-radius:12px; margin:8px 0; }\n"
        << "    .success { background:#ecfdf5; border:1px solid #a7f3d0; color:#065f46; padding:12px 14px; border-radius:12px; margin:8px 0; }\n"
        << "    .helper { color:var(--muted); font-size:13px; }\n"
        << "    footer { margin:28px 0 22px; color:var(--muted); font-size:14px; text-align:center; }\n"
        << "    @media (max-width:900px) { .card { padding:22px; } }\n"
        << "  </style>\n"
        << "</head>\n"
        << "<body class='" << layout << "'>\n"
        << "  <header>\n"
        << "    <div class='container'>\n"
        << "      <div class='nav'>\n"
        << "        <div class='brand'><span class='logo'></span><h1>Team Elevate Auctions</h1></div>\n"
        << "        <nav class='links'>\n";

    if (isLoggedIn) {
        std::cout << "          <a href='index.cgi'>Home</a>\n"
            "          <a href='list_auctions.cgi'>Browse Auctions</a>\n"
            "          <a href='my_bids.cgi'>My Transactions</a>\n"
            "          <a href='logout.cgi'>Logout</a>\n";
    }
    else {
        std::cout << "          <a href='index.cgi'>Home</a>\n"
            "          <a href='login.cgi'>Login</a>\n"
            "          <a href='register.cgi'>Register</a>\n";
    }

    std::cout << "        </nav>\n"
        "      </div>\n"
        "    </div>\n"
        "  </header>\n"
        "  <main>\n";

    if (layout == "content")
        std::cout << "    <div class='container'>\n";
}

void printTail(const std::string& layout) {
    if (layout == "content") {
        std::cout << "      <footer>&copy; 2025 Team Elevate. All rights reserved.</footer>\n"
            "    </div>\n";
    }
    else {
        std::cout << "      <footer>&copy; 2025 Team Elevate. All rights reserved.</footer>\n";
    }
    std::cout << "  </main>\n</body>\n</html>\n";
}

// ------------------------------------------------------------
// Check whether the current visitor has a valid active session
// ------------------------------------------------------------
bool isUserLoggedIn(std::string& userEmail) {
    char* httpCookie = std::getenv("HTTP_COOKIE");
    if (!httpCookie)
        return false;

    std::string sessionToken = getCookieValue(httpCookie, "session_token");
    if (sessionToken.empty())
        return false;

    MYSQL* conn = createDBConnection();
    if (!conn)
        return false;

    bool loggedIn = false;
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    const char* sql =
        "SELECT u.user_email, TIMESTAMPDIFF(SECOND, s.last_active, NOW()) "
        "FROM users u JOIN sessions s ON u.user_id = s.user_id "
        "WHERE s.session_token=? LIMIT 1";

    if (stmt && mysql_stmt_prepare(stmt, sql, std::strlen(sql)) == 0) {
        MYSQL_BIND param{};
        param.buffer_type = MYSQL_TYPE_STRING;
        param.buffer = (char*)sessionToken.c_str();
        param.buffer_length = sessionToken.size();

        if (mysql_stmt_bind_param(stmt, &param) == 0 && mysql_stmt_execute(stmt) == 0) {
            MYSQL_BIND result[2]{};
            char email_buf[256];
            unsigned long email_len = 0;
            unsigned long long inactive_seconds = 0;

            result[0].buffer_type = MYSQL_TYPE_STRING;
            result[0].buffer = email_buf;
            result[0].buffer_length = sizeof(email_buf);
            result[0].length = &email_len;

            result[1].buffer_type = MYSQL_TYPE_LONGLONG;
            result[1].buffer = &inactive_seconds;
            result[1].is_unsigned = 1;

            if (mysql_stmt_bind_result(stmt, result) == 0 &&
                mysql_stmt_store_result(stmt) == 0 &&
                mysql_stmt_fetch(stmt) == 0) {

                if (inactive_seconds <= 300) {
                    userEmail.assign(email_buf, email_len);
                    loggedIn = true;

                    // update last_active
                    MYSQL_STMT* update = mysql_stmt_init(conn);
                    const char* upd =
                        "UPDATE sessions SET last_active=NOW() WHERE session_token=?";
                    if (update && mysql_stmt_prepare(update, upd, std::strlen(upd)) == 0) {
                        MYSQL_BIND up{};
                        up.buffer_type = MYSQL_TYPE_STRING;
                        up.buffer = (char*)sessionToken.c_str();
                        up.buffer_length = sessionToken.size();
                        mysql_stmt_bind_param(update, &up);
                        mysql_stmt_execute(update);
                        mysql_stmt_close(update);
                    }
                }
                else {
                    // session expired -> delete it
                    MYSQL_STMT* del = mysql_stmt_init(conn);
                    const char* delsql =
                        "DELETE FROM sessions WHERE session_token=?";
                    if (del && mysql_stmt_prepare(del, delsql, std::strlen(delsql)) == 0) {
                        MYSQL_BIND dp{};
                        dp.buffer_type = MYSQL_TYPE_STRING;
                        dp.buffer = (char*)sessionToken.c_str();
                        dp.buffer_length = sessionToken.size();
                        mysql_stmt_bind_param(del, &dp);
                        mysql_stmt_execute(del);
                        mysql_stmt_close(del);
                    }
                }
            }
        }
        mysql_stmt_free_result(stmt);
        mysql_stmt_close(stmt);
    }
    closeDBConnection(conn);
    return loggedIn;
}