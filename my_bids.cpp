#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <functional>
#include <mysql/mysql.h>
#include <cstring>

#include "utils.hpp"

// -----------------------------------------------------------------------------
// Prepared-Statement Query
// -----------------------------------------------------------------------------
bool runQueryAndPrint(MYSQL* conn, const std::string& sql, MYSQL_BIND* params, unsigned int paramCount, const std::function<void(MYSQL_BIND*, unsigned long*)>& printRow, unsigned int expectedCols) {
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return false;
    if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    if (paramCount > 0 && mysql_stmt_bind_param(stmt, params) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }
    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_RES* meta = mysql_stmt_result_metadata(stmt);
    if (!meta) { mysql_stmt_close(stmt); return false; }
    unsigned int numCols = mysql_num_fields(meta);
    if (expectedCols > 0 && numCols != expectedCols) numCols = expectedCols;

    std::vector<std::vector<char>> buffers(numCols, std::vector<char>(256));
    std::vector<unsigned long> lengths(numCols);
    std::vector<MYSQL_BIND> result(numCols);
    memset(result.data(), 0, sizeof(MYSQL_BIND) * numCols);
    for (unsigned int i = 0; i < numCols; ++i) {
        result[i].buffer_type = MYSQL_TYPE_STRING;
        result[i].buffer = buffers[i].data();
        result[i].buffer_length = buffers[i].size();
        result[i].length = &lengths[i];
    }

    mysql_stmt_bind_result(stmt, result.data());
    mysql_stmt_store_result(stmt);

    bool any = false;
    while (mysql_stmt_fetch(stmt) == 0) {
        any = true;
        printRow(result.data(), lengths.data());
    }

    mysql_free_result(meta);
    mysql_stmt_close(stmt);
    return any;
}

int main() {
    std::cout << "Content-type: text/html\n\n";

    // --- SESSION VALIDATION ---
    std::string userEmail;
    int userId = -1;

    if (!isUserLoggedIn(userEmail)) {
        std::cout << "<!DOCTYPE html><html><head>" << "<meta http-equiv='refresh' content='0;url=login.cgi'>" << "</head><body></body></html>\n";
        return 0;
    }

    // Lookup user_id
    MYSQL* idConn = createDBConnection();
    if (idConn) {
        const char* sql = "SELECT user_id FROM users WHERE user_email=? LIMIT 1";
        MYSQL_STMT* stmt = mysql_stmt_init(idConn);
        if (stmt && mysql_stmt_prepare(stmt, sql, strlen(sql)) == 0) {
            MYSQL_BIND p{}; memset(&p, 0, sizeof(p));
            p.buffer_type = MYSQL_TYPE_STRING;
            p.buffer = (char*)userEmail.c_str();
            p.buffer_length = userEmail.size();
            mysql_stmt_bind_param(stmt, &p);
            if (mysql_stmt_execute(stmt) == 0) {
                MYSQL_BIND r{}; memset(&r, 0, sizeof(r));
                long id = 0; r.buffer_type = MYSQL_TYPE_LONG; r.buffer = &id;
                mysql_stmt_bind_result(stmt, &r);
                if (mysql_stmt_fetch(stmt) == 0) userId = id;
            }
            mysql_stmt_close(stmt);
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

    // -------------------------------------------------------------------------
    // HTML OUTPUT
    // -------------------------------------------------------------------------
    std::cout << R"(<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="refresh" content="305">
  <title>Team Elevate — My Transactions</title>
  <style>
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
    main { padding: 22px 0; }
    .card { background: var(--card); border-radius: var(--radius); box-shadow: var(--shadow); padding: 18px; border: 1px solid var(--border); }
    .muted { color: var(--muted); }
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
    table{ width: 100%; border-collapse: separate; border-spacing: 0; table-layout: fixed; }
    thead th{ background: #f9fafb; border-bottom: 1px solid var(--border); padding: 10px 12px; text-align: left; font-weight: 700; }
    tbody td{ padding: 12px; border-top: 1px solid var(--border); vertical-align: middle; }
    .cbids th+th, .cbids td+td{ border-left: 1px solid var(--border); }
    .cbids col:nth-child(1){ width: 44%; }
    .cbids col:nth-child(2){ width: 18%; }
    .cbids col:nth-child(3){ width: 18%; }
    .cbids col:nth-child(4){ width: 20%; }
    .cbids th:nth-child(2), .cbids th:nth-child(3),
    .cbids td:nth-child(2), .cbids td:nth-child(3){ text-align: center; }
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
      <section class="card">
        <h2 style="margin:0 0 8px">My Transactions</h2>
        <p class="muted">Your selling activity, purchases, current bids, and items you didn’t win.</p>
      </section>

      <section class="layout" aria-label="Transactions">
)";

    // -------------------------------------------------------------------------
    // CURRENT BIDS
    // -------------------------------------------------------------------------
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
        const char* sql =
            "SELECT i.item_id, i.title, "
            "IFNULL(FORMAT((SELECT MAX(b2.bid_amount) FROM bids b2 WHERE b2.item_id=i.item_id),2),'0.00'), "
            "IFNULL(FORMAT((SELECT MAX(b1.bid_amount) FROM bids b1 WHERE b1.item_id=i.item_id AND b1.bidder_id=?),2),'0.00') "
            "FROM items i WHERE i.end_time>NOW() AND EXISTS(SELECT 1 FROM bids bx WHERE bx.item_id=i.item_id AND bx.bidder_id=?) ORDER BY i.end_time ASC";
        MYSQL_BIND params[2]; memset(params, 0, sizeof(params));
        params[0].buffer_type = MYSQL_TYPE_LONG; params[0].buffer = &userId; params[0].is_unsigned = 1;
        params[1] = params[0];
        bool any = runQueryAndPrint(conn, sql, params, 2,
            [&](MYSQL_BIND* res, unsigned long*) {
                std::string item_id(htmlEscape((char*)res[0].buffer));
                std::string title(htmlEscape((char*)res[1].buffer));
                std::string highest(htmlEscape((char*)res[2].buffer));
                std::string yourmax(htmlEscape((char*)res[3].buffer));
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
            }, 4);
        if (!any) std::cout << "<tr><td colspan='4'>No active bids.</td></tr>\n";
    }

    // -------------------------------------------------------------------------
    // SELLING
    // -------------------------------------------------------------------------
    std::cout << "            </tbody>\n          </table>\n        </article>\n";
    std::cout << R"(        <article class="card sell">
          <h3 style="margin-top:0">Selling</h3>
          <table aria-label="Items you are selling">
            <thead><tr><th>Item</th><th>Status</th><th>Ends</th></tr></thead>
            <tbody>
)";
    {
        const char* sql =
            "SELECT title, CASE WHEN end_time<NOW() THEN 'Closed' ELSE 'Active' END, DATE_FORMAT(end_time,'%Y-%m-%d %H:%i') "
            "FROM items WHERE seller_id=? ORDER BY end_time DESC";
        MYSQL_BIND p[1]; memset(p, 0, sizeof(p));
        p[0].buffer_type = MYSQL_TYPE_LONG; p[0].buffer = &userId; p[0].is_unsigned = 1;
        bool any = runQueryAndPrint(conn, sql, p, 1,
            [&](MYSQL_BIND* res, unsigned long*) {
                std::string title(htmlEscape((char*)res[0].buffer));
                std::string status(htmlEscape((char*)res[1].buffer));
                std::string ends(htmlEscape((char*)res[2].buffer));
                std::cout << "<tr><td>" << title << "</td><td>" << status << "</td><td>" << ends << "</td></tr>\n";
            }, 3);
        if (!any) std::cout << "<tr><td colspan='3'>No listings.</td></tr>\n";
    }

    // -------------------------------------------------------------------------
    // PURCHASES
    // -------------------------------------------------------------------------
    std::cout << "            </tbody>\n          </table>\n        </article>\n";
    std::cout << R"(        <article class="card purch">
          <h3 style="margin-top:0">Purchases</h3>
          <table aria-label="Items you purchased">
            <thead><tr><th>Item</th><th>Winning Bid</th><th>Closed</th></tr></thead>
            <tbody>
)";
    {
        const char* sql =
            "SELECT i.title, IFNULL(FORMAT((SELECT bid_amount FROM bids WHERE bid_id=i.winning_bid_id),2),'0.00'), DATE_FORMAT(i.end_time,'%Y-%m-%d %H:%i') "
            "FROM items i WHERE i.winner_id=? ORDER BY i.end_time DESC";
        MYSQL_BIND p[1]; memset(p, 0, sizeof(p));
        p[0].buffer_type = MYSQL_TYPE_LONG; p[0].buffer = &userId; p[0].is_unsigned = 1;
        bool any = runQueryAndPrint(conn, sql, p, 1,
            [&](MYSQL_BIND* res, unsigned long*) {
                std::string title(htmlEscape((char*)res[0].buffer));
                std::string win_bid(htmlEscape((char*)res[1].buffer));
                std::string closed(htmlEscape((char*)res[2].buffer));
                std::cout << "<tr><td>" << title << "</td><td>$" << win_bid << "</td><td>" << closed << "</td></tr>\n";
            }, 3);
        if (!any) std::cout << "<tr><td colspan='3'>No purchases yet.</td></tr>\n";
    }

    // -------------------------------------------------------------------------
    // DIDN'T WIN
    // -------------------------------------------------------------------------
    std::cout << "            </tbody>\n          </table>\n        </article>\n";
    std::cout << R"(        <article class="card lost">
          <h3 style="margin-top:0">Didn’t Win</h3>
          <table aria-label="Auctions you didn’t win">
            <thead><tr><th>Item</th><th>Winning Bid</th><th>Closed</th></tr></thead>
            <tbody>
)";
    {
        const char* sql =
            "SELECT i.title, IFNULL(FORMAT((SELECT bid_amount FROM bids WHERE bid_id=i.winning_bid_id),2),'0.00'), DATE_FORMAT(i.end_time,'%Y-%m-%d %H:%i') "
            "FROM items i WHERE i.winner_id IS NOT NULL AND i.winner_id<>? AND EXISTS(SELECT 1 FROM bids b WHERE b.item_id=i.item_id AND b.bidder_id=?) ORDER BY i.end_time DESC";
        MYSQL_BIND params[2]; memset(params, 0, sizeof(params));
        params[0].buffer_type = MYSQL_TYPE_LONG; params[0].buffer = &userId; params[0].is_unsigned = 1;
        params[1] = params[0];
        bool any = runQueryAndPrint(conn, sql, params, 2,
            [&](MYSQL_BIND* res, unsigned long*) {
                std::string title(htmlEscape((char*)res[0].buffer));
                std::string win_bid(htmlEscape((char*)res[1].buffer));
                std::string closed(htmlEscape((char*)res[2].buffer));
                std::cout << "<tr><td>" << title << "</td><td>$" << win_bid << "</td><td>" << closed << "</td></tr>\n";
            }, 3);
        if (!any) std::cout << "<tr><td colspan='3'>No lost auctions.</td></tr>\n";
    }

    // -------------------------------------------------------------------------
    // END PAGE
    // -------------------------------------------------------------------------
    std::cout << "            </tbody>\n          </table>\n        </article>\n";
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
