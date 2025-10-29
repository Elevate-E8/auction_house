#include <iostream>
#include <cstdlib>
#include <string>
#include <mysql/mysql.h>
#include <cstring>

#include "utils.hpp"

int main() {
    std::cout << "Content-type: text/html\n\n";

    // --- SESSION VALIDATION ---
    std::string userEmail;
    int userId = -1;

    if (!isUserLoggedIn(userEmail)) {
        std::cout << "<!DOCTYPE html><html><head>"
            << "<meta http-equiv='refresh' content='0;url=login.cgi'>"
            << "</head><body></body></html>\n";
        return 0;
    }

    // Lookup user_id (for numeric queries)
    MYSQL* idConn = createDBConnection();
    if (idConn) {
        const std::string query =
            "SELECT user_id FROM users WHERE user_email='" + userEmail + "' LIMIT 1";
        if (mysql_query(idConn, query.c_str()) == 0) {
            MYSQL_RES* res = mysql_store_result(idConn);
            if (res) {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row && row[0]) userId = std::stoi(row[0]);
                mysql_free_result(res);
            }
        }
        closeDBConnection(idConn);
    }

    if (userId == -1) {
        std::cout << "<!DOCTYPE html><html><body><p>Unable to determine user ID.</p></body></html>\n";
        return 0;
    }

    // --- OPEN DB for page queries ---
    MYSQL* conn = createDBConnection();
    if (!conn) {
        std::cout << "<!DOCTYPE html><html><body><p>Database connection failed.</p></body></html>\n";
        return 0;
    }


    // ==================== HTML HEAD + HEADER (exact designer CSS/markup) ====================
    /* ---------- HTML OUTPUT (Full Frontend Layout) ---------- */
    std::cout << R"(<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="refresh" content="305">
  <title>Team Elevate — My Transactions</title>
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
      line-height: 1.6; color: var(--ink); background: var(--bg);
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

    /* ===== Header / Nav (kept from home page) ============================= */
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

    /* ===== Page structure/cards =========================================== */
    main { padding: 22px 0; }
    .card { background: var(--card); border-radius: var(--radius); box-shadow: var(--shadow); padding: 18px; border: 1px solid var(--border); }
    .muted { color: var(--muted); }

    /* Layout: full-width Current Bids on top; three cards under it */
    .layout{
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      grid-template-areas:
        "bids bids bids"
        "sell purch lost";
      gap: 16px;
      margin-top: 16px;
    }
    .bids{ grid-area: bids; }
    .sell{ grid-area: sell; }
    .purch{ grid-area: purch; }
    .lost{ grid-area: lost; }

    @media (max-width: 1060px){
      .layout{
        grid-template-columns: repeat(2, 1fr);
        grid-template-areas:
          "bids bids"
          "sell purch"
          "lost lost";
      }
    }
    @media (max-width: 720px){
      .layout{
        grid-template-columns: 1fr;
        grid-template-areas:
          "bids"
          "sell"
          "purch"
          "lost";
      }
    }

    /* ===== Tables ========================================================== */
    table{ width: 100%; border-collapse: separate; border-spacing: 0; table-layout: fixed; }
    thead th{ background: #f9fafb; border-bottom: 1px solid var(--border); padding: 10px 12px; text-align: left; font-weight: 700; }
    tbody td{ padding: 12px; border-top: 1px solid var(--border); vertical-align: middle; }
    /* subtle vertical dividers improve scan */
    .cbids th+th, .cbids td+td{ border-left: 1px solid var(--border); }

    /* Column widths for Current Bids */
    .cbids col:nth-child(1){ width: 44%; }
    .cbids col:nth-child(2){ width: 18%; }
    .cbids col:nth-child(3){ width: 18%; }
    .cbids col:nth-child(4){ width: 20%; }

    /* Center the numeric columns */
    .cbids th:nth-child(2), .cbids th:nth-child(3),
    .cbids td:nth-child(2), .cbids td:nth-child(3){ text-align: center; }

    /* ===== Action cell form: stays inside cell ============================ */
    .inline-form{
      display: flex; flex-wrap: wrap; gap: 10px; align-items: center; max-width: 100%;
    }
    .inline-form input[type="number"]{
      padding: 8px 10px;
      border: 1px solid var(--border); border-radius: 10px;
      flex: 1 1 140px; min-width: 0; width: auto;
    }
    .btn{
      display: inline-block; padding: 10px 14px; border-radius: 10px;
      text-decoration: none; font-weight: 700; border: 0; cursor: pointer;
    }
    .btn.primary{ background: var(--brand); color: #fff; }

    /* ===== Footer & helpers =============================================== */
    footer { margin: 28px 0 22px; color: var(--muted); font-size: 14px; }
    @media (prefers-reduced-motion: reduce) { * { transition: none !important; } }
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
          <a href="index.cgi">Home</a>
          <a href="list_auctions.cgi">Browse Auctions</a>
          <a href="my_bids.cgi" aria-current="page">My Transactions</a>
          <a href="logout.cgi">Logout</a>
        </nav>
      </div>
    </div>
  </header>

  <main id="main">
    <div class="container">
      <!-- Intro card (matches home page style) -->
      <section class="card">
        <h2 style="margin:0 0 8px">My Transactions</h2>
        <p class="muted">Your selling activity, purchases, current bids, and items you didn’t win.</p>
      </section>

      <section class="layout" aria-label="Transactions">
)";
    // -------- Current Bids (full width) --------
    std::cout << R"(        <article class="card bids">
          <h3 style="margin-top:0">Current Bids</h3>
          <table class="cbids" aria-label="Current bids">
            <colgroup><col><col><col><col></colgroup>
            <thead>
              <tr>
                <th>Item</th>
                <th>Highest Bid</th>
                <th>Your Max</th>
                <th>Action</th>
              </tr>
            </thead>
            <tbody>
)";

    {
        // Items still open where this user has bid; compute highest and user's max via subqueries
        char q[1024];
        snprintf(q, sizeof(q),
            "SELECT i.item_id, i.title, "
            "IFNULL(FORMAT((SELECT MAX(b2.bid_amount) FROM bids b2 WHERE b2.item_id=i.item_id),2),'0.00') AS highest, "
            "IFNULL(FORMAT((SELECT MAX(b1.bid_amount) FROM bids b1 WHERE b1.item_id=i.item_id AND b1.bidder_id=%d),2),'0.00') AS yourmax "
            "FROM items i "
            "WHERE i.end_time > NOW() "
            "AND EXISTS (SELECT 1 FROM bids bx WHERE bx.item_id=i.item_id AND bx.bidder_id=%d) "
            "ORDER BY i.end_time ASC",
            userId, userId);

        if (mysql_query(conn, q) == 0) {
            MYSQL_RES* res = mysql_store_result(conn);
            if (res) {
                unsigned long num = mysql_num_rows(res);
                if (num == 0) {
                    std::cout << "<tr><td colspan='4'>No active bids.</td></tr>\n";
                }
                else {
                    MYSQL_ROW row;
                    while ((row = mysql_fetch_row(res))) {
                        const char* item_id = row[0] ? row[0] : "";
                        const char* title = row[1] ? row[1] : "";
                        const char* highest = row[2] ? row[2] : "0.00";
                        const char* yourmax = row[3] ? row[3] : "0.00";

                        std::cout << "<tr>\n";
                        std::cout << "  <td>" << title << "</td>\n";
                        std::cout << "  <td>$" << highest << "</td>\n";
                        std::cout << "  <td>$" << yourmax << "</td>\n";
                        std::cout << "  <td>\n";
                        std::cout << "    <form class='inline-form' action='bid.cgi' method='post'>\n";
                        std::cout << "      <input type='hidden' name='item_id' value='" << item_id << "'>\n";
                        std::cout << "      <label class='visually-hidden' for='bid_" << item_id << "'>Increase max bid</label>\n";
                        std::cout << "      <input id='bid_" << item_id << "' name='bid_amount' type='number' step='0.01' min='" << highest << "' placeholder='Enter new max' required>\n";
                        std::cout << "      <button class='btn primary' type='submit'>Increase</button>\n";
                        std::cout << "    </form>\n";
                        std::cout << "  </td>\n";
                        std::cout << "</tr>\n";
                    }
                }
                mysql_free_result(res);
            }
            else {
                std::cout << "<tr><td colspan='4'>Error retrieving bids.</td></tr>\n";
            }
        }
        else {
            std::cout << "<tr><td colspan='4'>Error retrieving bids.</td></tr>\n";
        }
    }

    std::cout << "            </tbody>\n          </table>\n        </article>\n";

    // -------- Selling --------
    std::cout << R"(        <article class="card sell">
          <h3 style="margin-top:0">Selling</h3>
          <table aria-label="Items you are selling">
            <thead><tr><th>Item</th><th>Status</th><th>Ends</th></tr></thead>
            <tbody>
)";

    {
        char q[512];
        snprintf(q, sizeof(q),
            "SELECT title, "
            "CASE WHEN end_time < NOW() THEN 'Closed' ELSE 'Active' END AS status, "
            "DATE_FORMAT(end_time,'%%Y-%%m-%%d %%H:%%i') AS ends "
            "FROM items WHERE seller_id=%d ORDER BY end_time DESC",
            userId);

        if (mysql_query(conn, q) == 0) {
            MYSQL_RES* res = mysql_store_result(conn);
            if (res) {
                unsigned long num = mysql_num_rows(res);
                if (num == 0) {
                    std::cout << "<tr><td colspan='3'>No listings.</td></tr>\n";
                }
                else {
                    MYSQL_ROW row;
                    while ((row = mysql_fetch_row(res))) {
                        const char* title = row[0] ? row[0] : "";
                        const char* status = row[1] ? row[1] : "";
                        const char* ends = row[2] ? row[2] : "";
                        std::cout << "<tr><td>" << title << "</td><td>" << status << "</td><td>" << ends << "</td></tr>\n";
                    }
                }
                mysql_free_result(res);
            }
            else {
                std::cout << "<tr><td colspan='3'>Error retrieving listings.</td></tr>\n";
            }
        }
        else {
            std::cout << "<tr><td colspan='3'>Error retrieving listings.</td></tr>\n";
        }
    }

    std::cout << "            </tbody>\n          </table>\n        </article>\n";

    // -------- Purchases --------
    std::cout << R"(        <article class="card purch">
          <h3 style="margin-top:0">Purchases</h3>
          <table aria-label="Items you purchased">
            <thead><tr><th>Item</th><th>Winning Bid</th><th>Closed</th></tr></thead>
            <tbody>
)";

    {
        char q[768];
        snprintf(q, sizeof(q),
            "SELECT i.title, "
            "IFNULL(FORMAT((SELECT bid_amount FROM bids WHERE bid_id=i.winning_bid_id),2),'0.00') AS win_bid, "
            "DATE_FORMAT(i.end_time,'%%Y-%%m-%%d %%H:%%i') AS closed "
            "FROM items i WHERE i.winner_id=%d ORDER BY i.end_time DESC",
            userId);

        if (mysql_query(conn, q) == 0) {
            MYSQL_RES* res = mysql_store_result(conn);
            if (res) {
                unsigned long num = mysql_num_rows(res);
                if (num == 0) {
                    std::cout << "<tr><td colspan='3'>No purchases yet.</td></tr>\n";
                }
                else {
                    MYSQL_ROW row;
                    while ((row = mysql_fetch_row(res))) {
                        const char* title = row[0] ? row[0] : "";
                        const char* price = row[1] ? row[1] : "0.00";
                        const char* closed = row[2] ? row[2] : "";
                        std::cout << "<tr><td>" << title << "</td><td>$" << price << "</td><td>" << closed << "</td></tr>\n";
                    }
                }
                mysql_free_result(res);
            }
            else {
                std::cout << "<tr><td colspan='3'>Error retrieving purchases.</td></tr>\n";
            }
        }
        else {
            std::cout << "<tr><td colspan='3'>Error retrieving purchases.</td></tr>\n";
        }
    }

    std::cout << "            </tbody>\n          </table>\n        </article>\n";

    // -------- Didn’t Win --------
    std::cout << R"(        <article class="card lost">
          <h3 style="margin-top:0">Didn’t Win</h3>
          <table aria-label="Auctions you didn’t win">
            <thead><tr><th>Item</th><th>Winning Bid</th><th>Closed</th></tr></thead>
            <tbody>
)";

    {
        char q[1024];
        snprintf(q, sizeof(q),
            "SELECT i.title, "
            "IFNULL(FORMAT((SELECT bid_amount FROM bids WHERE bid_id=i.winning_bid_id),2),'0.00') AS win_bid, "
            "DATE_FORMAT(i.end_time,'%%Y-%%m-%%d %%H:%%i') AS closed "
            "FROM items i "
            "WHERE i.winner_id IS NOT NULL AND i.winner_id<>%d "
            "AND EXISTS (SELECT 1 FROM bids b WHERE b.item_id=i.item_id AND b.bidder_id=%d) "
            "ORDER BY i.end_time DESC",
            userId, userId);

        if (mysql_query(conn, q) == 0) {
            MYSQL_RES* res = mysql_store_result(conn);
            if (res) {
                unsigned long num = mysql_num_rows(res);
                if (num == 0) {
                    std::cout << "<tr><td colspan='3'>No lost auctions.</td></tr>\n";
                }
                else {
                    MYSQL_ROW row;
                    while ((row = mysql_fetch_row(res))) {
                        const char* title = row[0] ? row[0] : "";
                        const char* price = row[1] ? row[1] : "0.00";
                        const char* closed = row[2] ? row[2] : "";
                        std::cout << "<tr><td>" << title << "</td><td>$" << price << "</td><td>" << closed << "</td></tr>\n";
                    }
                }
                mysql_free_result(res);
            }
            else {
                std::cout << "<tr><td colspan='3'>Error retrieving lost auctions.</td></tr>\n";
            }
        }
        else {
            std::cout << "<tr><td colspan='3'>Error retrieving lost auctions.</td></tr>\n";
        }
    }

    std::cout << "            </tbody>\n          </table>\n        </article>\n";

    // -------- Close layout / page --------
    std::cout << R"(      </section>
      <footer>&copy; 2025 Team Elevate. All rights reserved.</footer>
    </div>
  </main>
</body>
</html>
)";

    closeDBConnection(conn);
    return 0;
}