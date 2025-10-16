#include <iostream>
#include <cstdlib>
#include <string>
#include <mysql/mysql.h>
#include <cstring>

using namespace std;

// URL decode helper
string urlDecode(const string& str) {
    string result;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '+') result += ' ';
        else if (str[i] == '%' && i + 2 < str.length()) {
            unsigned int value;
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &value);
            result += static_cast<char>(value);
            i += 2;
        }
        else result += str[i];
    }
    return result;
}

int main() {
    cout << "Content-type: text/html\n\n";

    // --- COOKIE READ ---
    char* httpCookie = getenv("HTTP_COOKIE");
    string sessionToken;
    string userEmail;
    bool isLoggedIn = false;

    if (httpCookie) {
        string cookies(httpCookie);
        size_t pos = cookies.find("session_token=");
        if (pos != string::npos) {
            size_t start = pos + 14;
            size_t end = cookies.find(";", start);
            if (end == string::npos) end = cookies.length();
            sessionToken = cookies.substr(start, end - start);
        }
    }

    // --- SESSION VERIFICATION ---
    if (!sessionToken.empty()) {
        MYSQL* conn = mysql_init(NULL);
        if (conn && mysql_real_connect(conn, "localhost", "cs370_section2_elevate",
            "etavele_004", "cs370_section2_elevate", 0, NULL, 0)) {

            MYSQL_STMT* stmt = mysql_stmt_init(conn);
            const char* sql = "SELECT u.user_email, TIMESTAMPDIFF(SECOND, s.last_active, NOW()) "
                "FROM users u JOIN sessions s ON u.user_id = s.user_id "
                "WHERE s.session_token = ? LIMIT 1";

            if (stmt && mysql_stmt_prepare(stmt, sql, strlen(sql)) == 0) {
                MYSQL_BIND param{};
                param.buffer_type = MYSQL_TYPE_STRING;
                param.buffer = (char*)sessionToken.c_str();
                param.buffer_length = sessionToken.size();

                if (mysql_stmt_bind_param(stmt, &param) == 0 && mysql_stmt_execute(stmt) == 0) {
                    MYSQL_BIND result[2]{};
                    char email_buf[256];
                    unsigned long email_len = 0;
                    my_ulonglong inactive_seconds = 0;

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
                            isLoggedIn = true;

                            MYSQL_STMT* updateStmt = mysql_stmt_init(conn);
                            const char* updateSql = "UPDATE sessions SET last_active = NOW() WHERE session_token = ?";
                            if (updateStmt && mysql_stmt_prepare(updateStmt, updateSql, strlen(updateSql)) == 0) {
                                MYSQL_BIND updateParam{};
                                updateParam.buffer_type = MYSQL_TYPE_STRING;
                                updateParam.buffer = (char*)sessionToken.c_str();
                                updateParam.buffer_length = sessionToken.size();
                                mysql_stmt_bind_param(updateStmt, &updateParam);
                                mysql_stmt_execute(updateStmt);
                                mysql_stmt_close(updateStmt);
                            }
                        }
                        else {
                            MYSQL_STMT* delStmt = mysql_stmt_init(conn);
                            const char* delSql = "DELETE FROM sessions WHERE session_token = ?";
                            if (delStmt && mysql_stmt_prepare(delStmt, delSql, strlen(delSql)) == 0) {
                                MYSQL_BIND delParam{};
                                delParam.buffer_type = MYSQL_TYPE_STRING;
                                delParam.buffer = (char*)sessionToken.c_str();
                                delParam.buffer_length = sessionToken.size();
                                mysql_stmt_bind_param(delStmt, &delParam);
                                mysql_stmt_execute(delStmt);
                                mysql_stmt_close(delStmt);
                            }
                        }
                    }
                }
                mysql_stmt_free_result(stmt);
                mysql_stmt_close(stmt);
            }
            mysql_close(conn);
        }
    }

    // --- HTML OUTPUT ---
    cout << R"(<!doctype html>
    <html lang="en">
    <head>
      <meta charset="utf-8">
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <meta http-equiv="refresh" content="305">
      <title>Team Elevate — Auctions</title>
      <style>
        /* ===== Theme & Resets ================================================= */
        :root {
          --ink: #0f172a;
          --muted: #475569;
          --bg: #f6f7fb;
          --card: #ffffff;
          --brand: #4f46e5;
          --brand-2: #22c55e;
          --border: #e5e7eb;
          --shadow: 0 6px 18px rgba(2, 6, 23, .08);
          --radius: 14px;
        }

        *, *::before, *::after { box-sizing: border-box; }
        html, body { height: 100%; }
        body {
          margin: 0; padding: 0;
          font-family: system-ui, -apple-system, Segoe UI, Roboto, Arial, sans-serif;
          line-height: 1.6;
          color: var(--ink);
          background: var(--bg);
          -webkit-font-smoothing: antialiased; -moz-osx-font-smoothing: grayscale;
        }

        img { max-width: 100%; display: block; }
        a { color: inherit; text-underline-offset: 2px; }
        a:focus-visible, button:focus-visible { outline: 2px solid var(--brand); outline-offset: 2px; }

        .container { max-width: 1100px; margin-inline: auto; padding-inline: 20px; }
        .visually-hidden { position: absolute; clip: rect(0 0 0 0); clip-path: inset(50%); width: 1px; height: 1px; overflow: hidden; white-space: nowrap; }

        .skip-link {
          position: absolute; left: 12px; top: -40px;
          background: var(--brand); color: #fff; padding: 8px 12px; border-radius: 10px;
          transition: top .2s ease;
        }
        .skip-link:focus { top: 12px; }

        /* ===== Header / Nav =================================================== */
        header {
          background: linear-gradient(135deg, #111827, #1f2937 50%, #111827);
          color: #fff;
        }

        .nav { display: flex; align-items: center; justify-content: space-between; padding: 18px 0; gap: 16px; }
        .brand { display: flex; align-items: center; gap: 12px; }
        .logo { width: 36px; height: 36px; border-radius: 10px; background: linear-gradient(135deg, var(--brand), #7c3aed); }
        .brand h1 { margin: 0; font-size: 18px; letter-spacing: .3px; }

        .links { display: flex; gap: 14px; align-items: center; flex-wrap: wrap; }
        .links a { color: #e5e7eb; text-decoration: none; font-weight: 600; padding: 6px 8px; border-radius: 8px; }
        .links a:hover { color: #fff; background: rgba(255,255,255,.08); }

        /* ===== Hero =========================================================== */
        .hero { display: grid; grid-template-columns: 1.2fr .8fr; gap: 28px; align-items: center; padding: 28px 0 42px; }
        .hero h2 { margin: 0 0 12px; font-size: clamp(28px, 4vw, 36px); line-height: 1.15; }
        .hero p { margin: 0; color: #cbd5e1; }

        .cta { margin-top: 18px; display: flex; gap: 12px; flex-wrap: wrap; }
        .btn { display: inline-block; padding: 10px 14px; border-radius: 10px; text-decoration: none; font-weight: 700; }
        .btn.primary { background: var(--brand); color: #fff; }
        .btn.ghost { background: rgba(255,255,255,.1); color: #fff; border: 1px solid rgba(255,255,255,.25); }

        /* ===== Cards & Layout ================================================= */
        main { padding: 22px 0; }
        .card { background: var(--card); border-radius: var(--radius); box-shadow: var(--shadow); padding: 18px; border: 1px solid var(--border); }
        .card h3 { margin-top: 0; color: #374151; }

        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 16px; margin-top: 16px; }
        .feature h3 { margin: 0 0 6px; font-size: 18px; }
        .muted { color: var(--muted); }

        .user-info { background: #eef6ff; padding: 10px 12px; border-radius: 10px; margin: 14px 0; border: 1px solid #dbeafe; }

        footer { margin: 28px 0 22px; color: var(--muted); font-size: 14px; }

        /* ===== Motion preferences ============================================ */
        @media (prefers-reduced-motion: reduce) { * { transition: none !important; } }

        /* ===== Breakpoints ==================================================== */
        @media (max-width: 900px) { .hero { grid-template-columns: 1fr; } }
      </style>
    </head>
    <body>
      <a class="skip-link" href="#main">Skip to content</a>
      <header>
        <div class="container">
          <div class="nav" role="navigation" aria-label="Primary">
            <div class="brand">
              <span class="logo" aria-hidden="true"></span>
              <h1>Team Elevate Auctions</h1>
            </div>
            <nav class="links">
    )";

    // --- Dynamic links based on login ---
    if (isLoggedIn) {
      cout << R"(          <a href='index.cgi'>Home</a>
              <a href='list_auctions.cgi'>Browse Auctions</a>
              <a href='my_bids.cgi'>My Bids</a>
              <a href='logout.cgi'>Logout</a>
    )";
    } else {
      cout << R"(          <a href='index.cgi'>Home</a>
              <a href='login.cgi'>Login</a>
              <a href='register.cgi'>Register</a>
    )";
    }

    cout << R"(        </nav>
          </div>

          <section class="hero" aria-label="Welcome">
            <div>
              <h2>Bid. Win. Elevate.</h2>
              <p>Trusted listings, transparent bidding, and real-time results — all in one place.</p>
              <div class="cta">
    )";

    if (!isLoggedIn) {
      cout << R"(            <a class='btn primary' href='register.cgi'>Create an account</a>
                <a class='btn ghost' href='login.cgi'>Log in</a>
    )";
    } else {
      cout << R"(            <a class='btn primary' href='list_auctions.cgi'>Browse auctions</a>
                <a class='btn ghost' href='my_bids.cgi'>View my bids</a>
    )";
    }

    cout << R"(          </div>
            </div>

            <aside class="card" aria-label="Today's Highlights">
              <h3>Today's Highlights</h3>
              <ul style="margin:8px 0 0; padding-left:18px; color:#374151;">
                <li>Live updates as bids change</li>
                <li>Verified sellers &amp; secure checkout</li>
                <li>Watchlist to track your favorites</li>
              </ul>
            </aside>
          </section>
        </div>
      </header>

      <main id="main">
        <div class="container">
    )";

    if (isLoggedIn) {
      cout << "      <div class='user-info'>\n"
              "        ✓ Logged in as: <strong>" << userEmail << "</strong>\n"
              "      </div>\n";
    }

    cout << R"(      <section class="card">
            <h2 style="margin-top:0">Why choose Team Elevate?</h2>
            <div class="grid">
              <div class="feature">
                <h3>Fair &amp; Transparent</h3>
                <p class="muted">Clear rules, visible bid history, and no hidden fees.</p>
              </div>
              <div class="feature">
                <h3>Fast &amp; Secure</h3>
                <p class="muted">Optimized checkout and encrypted sessions keep you safe.</p>
              </div>
              <div class="feature">
                <h3>Built for You</h3>
                <p class="muted">Personalized watchlists and alerts so you never miss a win.</p>
              </div>
            </div>
          </section>

          <footer>
            &copy; 2025 Team Elevate. All rights reserved.
          </footer>
        </div>
      </main>
    </body>
    </html>
    )";

    return 0;
}
