#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <mysql/mysql.h>
using namespace std;

// ----------------- Get cookie value helper -----------------
string getCookieValue(const string& cookieHeader, const string& name) {
    size_t start = cookieHeader.find(name + "=");
    if (start == string::npos) return "";
    start += name.length() + 1;
    size_t end = cookieHeader.find(";", start);
    if (end == string::npos) end = cookieHeader.length();
    return cookieHeader.substr(start, end - start);
}

// ----------------- Main -----------------
int main() {
    // Get session_token from cookies
    char* cookieEnv = getenv("HTTP_COOKIE");
    string sessionToken;
    if (cookieEnv) {
        sessionToken = getCookieValue(string(cookieEnv), "session_token");
    }

    // Delete session from DB if token exists
    if (!sessionToken.empty()) {
        MYSQL* conn = mysql_init(NULL);
        if (conn && mysql_real_connect(conn, "localhost", "cs370_section2_elevate",
            "etavele_004", "cs370_section2_elevate", 0, NULL, 0)) {

            MYSQL_STMT* stmt = mysql_stmt_init(conn);
            const char* sql = "DELETE FROM sessions WHERE session_token=?";
            if (stmt && mysql_stmt_prepare(stmt, sql, strlen(sql)) == 0) {
                MYSQL_BIND param[1];
                memset(param, 0, sizeof(param));
                param[0].buffer_type = MYSQL_TYPE_STRING;
                param[0].buffer = (char*)sessionToken.c_str();
                param[0].buffer_length = sessionToken.size();
                mysql_stmt_bind_param(stmt, param);
                mysql_stmt_execute(stmt);
                mysql_stmt_close(stmt);
            }

            mysql_close(conn);
        }
    }

    // ----------------- HEADERS -----------------
    cout << "Content-Type: text/html\r\n";
    cout << "Set-Cookie: session_token=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Max-Age=0\r\n";
    cout << "Cache-Control: no-store, no-cache, must-revalidate\r\n";
    cout << "Pragma: no-cache\r\n";
    cout << "\r\n"; // end headers

    // ----------------- HTML RESPONSE -----------------
    cout
        << "<!DOCTYPE html>\n"
        "<html lang='en'>\n"
        "<head>\n"
        "  <meta charset='utf-8'>\n"
        "  <meta name='viewport' content='width=device-width, initial-scale=1'>\n"
        "  <meta http-equiv='refresh' content='2; url=index.cgi'>\n"
        "  <title>Logged Out · Team Elevate</title>\n"
        "  <style>\n"
        "    :root{--ink:#0f172a;--muted:#475569;--bg:#f6f7fb;--card:#ffffff;--brand:#4f46e5}\n"
        "    *{box-sizing:border-box}\n"
        "    html,body{margin:0;padding:0}\n"
        "    body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;color:var(--ink);background:var(--bg);line-height:1.5}\n"
        "    header{background:linear-gradient(135deg,#111827,#1f2937 50%,#111827);color:#fff}\n"
        "    .wrap{max-width:1024px;margin:0 auto;padding:0 20px}\n"
        "    .nav{display:flex;align-items:center;justify-content:space-between;padding:18px 0}\n"
        "    .brand{display:flex;gap:10px;align-items:center}\n"
        "    .logo{width:36px;height:36px;border-radius:10px;background:linear-gradient(135deg,var(--brand),#7c3aed)}\n"
        "    .brand h1{font-size:18px;margin:0;font-weight:700;letter-spacing:.3px}\n"
        "    .links a{color:#e5e7eb;text-decoration:none;margin-left:14px;font-weight:600}\n"
        "    .links a:hover{color:#fff;text-decoration:underline}\n"
        "    main{padding:28px 0; display:grid; place-items:center}\n"
        "    .card{background:var(--card);border-radius:16px;box-shadow:0 8px 22px rgba(2,6,23,.10);padding:24px;width:100%;max-width:520px;text-align:center}\n"
        "    .muted{color:var(--muted)}\n"
        "    a.btn{display:inline-block;margin-top:12px;padding:10px 14px;border-radius:10px;background:var(--brand);color:#fff;font-weight:700;text-decoration:none}\n"
        "    a.btn:hover{filter:brightness(1.05)}\n"
        "  </style>\n"
        "</head>\n"
        "<body>\n"
        "  <header>\n"
        "    <div class='wrap'>\n"
        "      <div class='nav'>\n"
        "        <div class='brand'>\n"
        "          <span class='logo' aria-hidden='true'></span>\n"
        "          <h1>Team Elevate Auctions</h1>\n"
        "        </div>\n"
        "        <nav class='links'>\n"
        "          <a href='index.cgi'>Home</a>\n"
        "          <a href='login.cgi'>Login</a>\n"
        "          <a href='register.cgi'>Register</a>\n"
        "        </nav>\n"
        "      </div>\n"
        "    </div>\n"
        "  </header>\n"
        "  <main>\n"
        "    <div class='card'>\n"
        "      <h1>✓ Signed out</h1>\n"
        "      <p class='muted'>Your session has ended. We’re taking you back to the homepage…</p>\n"
        "      <a class='btn' href='index.cgi'>Go to Home</a>\n"
        "    </div>\n"
        "  </main>\n"
        "</body>\n"
        "</html>\n";

    return 0;
}
