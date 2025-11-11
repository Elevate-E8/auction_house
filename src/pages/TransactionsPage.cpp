// pages/TransactionsPage.cpp
#include "pages/TransactionsPage.hpp"
#include "utils/utils.hpp"
#include <iostream>
#include <vector>
#include <cstring>
#include <functional>

TransactionsPage::TransactionsPage(Database& db, Session& session)
    : Page(db, session) {
}

// Helper to run prepared queries and print rows
static bool runQueryAndPrint(
    MYSQL* conn,
    const std::string& sql,
    MYSQL_BIND* params,
    unsigned int paramCount,
    const std::function<void(MYSQL_BIND*, unsigned long*)>& printRow,
    unsigned int expectedCols = 0)
{
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

void TransactionsPage::handleGet() {
    // Redirect BEFORE sending any HTML so we don't need a <meta http-equiv="refresh"> in body
    if (!session_.isLoggedIn()) {
        std::cout << "Status: 302 Found\r\nLocation: login.cgi\r\n\r\n";
        return;
    }

    sendHTMLHeader();
    printHead("My Transactions · Team Elevate", "content");

    MYSQL* conn = db_.connection();
    if (!conn) {
        std::cout << "<div class='error'>Database connection failed.</div>\n";
        printTail("content");
        return;
    }

    long userId = session_.userId();
    std::string email = session_.userEmail();

    // ---------------------------------------------------------------------
    // Page header + tiny tab styles
    // ---------------------------------------------------------------------
    std::cout << R"(
    <section class="card">
      
      <h1 style="margin:0 0 8px">My Transactions</h1>
      <p class="muted">Your selling activity, purchases, current bids, and items you didn't win.</p>

      <div role="tablist" aria-label="Transactions Tabs" class="tx-tabs" style="display:flex; gap:10px; flex-wrap:wrap; margin-top:14px;">
        <button role="tab" class="tx-tab" data-target="tab-selling"      aria-selected="true">Selling</button>
        <button role="tab" class="tx-tab" data-target="tab-purchases"    aria-selected="false">Purchases</button>
        <button role="tab" class="tx-tab" data-target="tab-bids"         aria-selected="false">Current Bids</button>
        <button role="tab" class="tx-tab" data-target="tab-lost"         aria-selected="false">Didn't Win</button>
      </div>

      <style>
        .tx-tab{
          border:1px solid var(--border);
          background:#fff;
          padding:8px 14px; border-radius:999px;
          font-weight:700; cursor:pointer;
        }
        .tx-tab[aria-selected="true"]{
          outline:2px solid #fb923c;
          outline-offset:2px;
        }
        .tx-section{ display:none; margin-top:16px; }
        .tx-section.active{ display:block; }
        .muted{ color:#6b7280; }

        /* ---- Tables ---- */
        table{ width:100%; border-collapse:separate; border-spacing:0; }
        th, td{ padding:14px 16px; vertical-align:middle; }
        thead th{ background:#f3f4f6; font-weight:700; text-align:left; }

        /* Current Bids layout fixes */
        .cbids{ table-layout:fixed; }
        .cbids col:nth-child(1){ width:38%; } /* Item */
        .cbids col:nth-child(2){ width:18%; } /* Current Leader */
        .cbids col:nth-child(3){ width:16%; } /* Highest Bid */
        .cbids col:nth-child(4){ width:16%; } /* Your Max */
        .cbids col:nth-child(5){ width:12%; min-width:160px; } /* Action */

        .name-wrap{
          display:block;
          white-space:normal;
          overflow-wrap:anywhere;
          line-height:1.35;
        }

        .action-cell{
          display:flex;
          align-items:center;
          gap:10px;
          justify-content:flex-start;
          width:100%;
        }
        .inline-form{ display:flex; align-items:center; gap:10px; width:100%; }
        .inline-form input[type=number]{ min-width:9rem; max-width:100%; }
        .inline-form .btn{ white-space:nowrap; }
      </style>
    </section>
)";

    // =====================================================
    // SELLING 
    // =====================================================
    std::cout << R"(
    <section id="tab-selling" class="card tx-section active" aria-labelledby="Selling">
      <h3 style="margin-top:0">Selling</h3>
      <table aria-label="Items you are selling" class="selling">
        <thead>
          <tr>
            <th>Item</th><th>Status</th><th>Ends</th><th>Current Bidder</th><th>Highest Bid</th>
          </tr>
        </thead>
        <tbody>
)";
    {
        const char* sql =
            "SELECT title, "
            "CASE WHEN end_time<NOW() THEN 'Closed' ELSE 'Active' END, "
            "DATE_FORMAT(end_time,'%m/%d/%Y %h:%i %p'), "
            "UNIX_TIMESTAMP(end_time) "
            "FROM items WHERE seller_id=? ORDER BY end_time DESC";
        MYSQL_BIND p[1]; memset(p, 0, sizeof(p));
        p[0].buffer_type = MYSQL_TYPE_LONG; p[0].buffer = &userId; p[0].is_unsigned = 1;

        bool any = runQueryAndPrint(conn, sql, p, 1,
            [&](MYSQL_BIND* res, unsigned long*) {
                std::string title(htmlEscape((char*)res[0].buffer));
                std::string status(htmlEscape((char*)res[1].buffer));
                std::string endsServer(htmlEscape((char*)res[2].buffer));
                std::string epoch(htmlEscape((char*)res[3].buffer));

                // Placeholders (backend will fill these later)
                std::string leader = "—";
                std::string highest = "0.00";

                std::cout << "<tr>"
                          << "<td><span class='name-wrap'>" << title << "</span></td>"
                          << "<td>" << status << "</td>"
                          << "<td><time class='dt' data-epoch='" << epoch << "'>" << endsServer << "</time></td>"
                          << "<td>" << leader << "</td>"
                          << "<td>$" << highest << "</td>"
                          << "</tr>\n";
            }, 4);
        if (!any) std::cout << "<tr><td colspan='5'>No listings.</td></tr>\n";
    }
    std::cout << "</tbody></table></section>\n";

    // =====================================================
    // PURCHASES
    // =====================================================
    std::cout << R"(
    <section id="tab-purchases" class="card tx-section" aria-labelledby="Purchases">
      <h3 style="margin-top:0">Purchases</h3>
      <table aria-label="Items you purchased">
        <thead><tr><th>Item</th><th>Winning Bid</th><th>Closed</th></tr></thead>
        <tbody>
)";
    {
        const char* sql =
            "SELECT i.title, "
            "IFNULL(FORMAT((SELECT bid_amount FROM bids WHERE bid_id=i.winning_bid_id),2),'0.00'), "
            "DATE_FORMAT(i.end_time,'%m/%d/%Y %h:%i %p'), "
            "UNIX_TIMESTAMP(i.end_time) "
            "FROM items i WHERE i.winner_id=? ORDER BY i.end_time DESC";
        MYSQL_BIND p[1]; memset(p, 0, sizeof(p));
        p[0].buffer_type = MYSQL_TYPE_LONG; p[0].buffer = &userId; p[0].is_unsigned = 1;

        bool any = runQueryAndPrint(conn, sql, p, 1,
            [&](MYSQL_BIND* res, unsigned long*) {
                std::string title(htmlEscape((char*)res[0].buffer));
                std::string bid(htmlEscape((char*)res[1].buffer));
                std::string closedServer(htmlEscape((char*)res[2].buffer));
                std::string epoch(htmlEscape((char*)res[3].buffer));
                std::cout << "<tr><td><span class='name-wrap'>" << title
                          << "</span></td><td>$" << bid
                          << "</td><td><time class='dt' data-epoch='" << epoch << "'>" << closedServer << "</time></td></tr>\n";
            }, 4);
        if (!any) std::cout << "<tr><td colspan='3'>No purchases yet.</td></tr>\n";
    }
    std::cout << "</tbody></table></section>\n";

    // =====================================================
    // CURRENT BIDS 
    // =====================================================
    std::cout << R"(
    <section id="tab-bids" class="card tx-section" aria-labelledby="Current Bids">
      <h3 style="margin-top:0">Current Bids</h3>
      <table class="cbids" aria-label="Current bids">
        <colgroup><col><col><col><col><col></colgroup>
        <thead>
          <tr><th>Item</th><th>Current Leader</th><th>Highest Bid</th><th>Your Max</th><th>Action</th></tr>
        </thead>
        <tbody>
)";
    {
        const char* sql =
            "SELECT i.item_id, i.title, "
            "IFNULL(FORMAT((SELECT MAX(b2.bid_amount) FROM bids b2 WHERE b2.item_id=i.item_id),2),'0.00'), "
            "IFNULL(FORMAT((SELECT MAX(b1.bid_amount) FROM bids b1 WHERE b1.item_id=i.item_id AND b1.bidder_id=?),2),'0.00') "
            "FROM items i WHERE i.end_time>NOW() "
            "AND EXISTS(SELECT 1 FROM bids bx WHERE bx.item_id=i.item_id AND bx.bidder_id=?) "
            "ORDER BY i.end_time ASC";

        MYSQL_BIND params[2]; memset(params, 0, sizeof(params));
        params[0].buffer_type = MYSQL_TYPE_LONG; params[0].buffer = &userId; params[0].is_unsigned = 1;
        params[1] = params[0];

        bool any = runQueryAndPrint(conn, sql, params, 2,
            [&](MYSQL_BIND* res, unsigned long*) {
                std::string item_id(htmlEscape((char*)res[0].buffer));
                std::string title(htmlEscape((char*)res[1].buffer));
                std::string highest(htmlEscape((char*)res[2].buffer));
                std::string yourmax(htmlEscape((char*)res[3].buffer));
                std::string leader = "—"; // backend to populate

                std::cout << "<tr>\n"
                    << "  <td><span class='name-wrap'>" << title << "</span></td>\n"
                    << "  <td>" << leader << "</td>\n"
                    << "  <td>$" << highest << "</td>\n"
                    << "  <td>$" << yourmax << "</td>\n"
                    << "  <td>\n"
                    << "    <div class='action-cell'>\n"
                    << "      <form class='inline-form' action='bid.cgi' method='post'>\n"
                    << "        <input type='hidden' name='item_id' value='" << item_id << "'>\n"
                    << "        <input name='bid_amount' type='number' step='0.01' placeholder='Enter new max' required>\n"
                    << "        <button class='btn primary' type='submit'>Increase</button>\n"
                    << "      </form>\n"
                    << "    </div>\n"
                    << "  </td>\n"
                    << "</tr>\n";
            }, 4);
        if (!any) std::cout << "<tr><td colspan='5'>No active bids.</td></tr>\n";
    }
    std::cout << "</tbody></table></section>\n";

    // =====================================================
    // LOST 
    // =====================================================
    std::cout << R"(
    <section id="tab-lost" class="card tx-section" aria-labelledby="Didn't Win">
      <h3 style="margin-top:0">Didn't Win</h3>
      <table aria-label="Auctions you didn't win">
        <thead><tr><th>Item</th><th>Winning Bid</th><th>Closed</th></tr></thead>
        <tbody>
)";
    {
        const char* sql =
            "SELECT i.title, "
            "IFNULL(FORMAT((SELECT bid_amount FROM bids WHERE bid_id=i.winning_bid_id),2),'0.00'), "
            "DATE_FORMAT(i.end_time,'%m/%d/%Y %h:%i %p'), "
            "UNIX_TIMESTAMP(i.end_time) "
            "FROM items i WHERE i.winner_id IS NOT NULL AND i.winner_id<>? "
            "AND EXISTS(SELECT 1 FROM bids b WHERE b.item_id=i.item_id AND b.bidder_id=?) "
            "ORDER BY i.end_time DESC";
        MYSQL_BIND params[2]; memset(params, 0, sizeof(params));
        params[0].buffer_type = MYSQL_TYPE_LONG; params[0].buffer = &userId; params[0].is_unsigned = 1;
        params[1] = params[0];

        bool any = runQueryAndPrint(conn, sql, params, 2,
            [&](MYSQL_BIND* res, unsigned long*) {
                std::string title(htmlEscape((char*)res[0].buffer));
                std::string bid(htmlEscape((char*)res[1].buffer));
                std::string closedServer(htmlEscape((char*)res[2].buffer));
                std::string epoch(htmlEscape((char*)res[3].buffer));
                std::cout << "<tr><td><span class='name-wrap'>" << title
                          << "</span></td><td>$" << bid
                          << "</td><td><time class='dt' data-epoch='" << epoch << "'>" << closedServer << "</time></td></tr>\n";
            }, 4);
        if (!any) std::cout << "<tr><td colspan='3'>No lost auctions.</td></tr>\n";
    }
    std::cout << "</tbody></table></section>\n";

    // ---------------------------------------------------------------------
    // JS: tab switcher + Pacific Time formatter
    // ---------------------------------------------------------------------
    std::cout << R"(
    <script>
    (function(){
      function qs(name){
        var m = new RegExp('[?&]'+name+'=([^&]+)').exec(location.search);
        return m ? decodeURIComponent(m[1].replace(/\+/g,' ')) : null;
      }
      var tabs = document.querySelectorAll('.tx-tab');
      var sections = document.querySelectorAll('.tx-section');
      function activate(id){
        sections.forEach(function(s){ s.classList.toggle('active', s.id===id); });
        tabs.forEach(function(t){ t.setAttribute('aria-selected', t.dataset.target===id ? 'true' : 'false'); });
      }
      tabs.forEach(function(btn){
        btn.addEventListener('click', function(){
          var id = btn.dataset.target;
          var url = new URL(window.location.href);
          url.searchParams.set('tab', id.replace('tab-',''));
          history.replaceState(null,'',url);
          activate(id);
        });
      });
      var initial = qs('tab');
      if(initial){
        var id = 'tab-' + initial;
        if(document.getElementById(id)) activate(id);
      }
    })();
    // Pacific Time (California) formatter for <time class="dt" data-epoch="...">
    (function(){
      var fmt = new Intl.DateTimeFormat('en-US', {
        timeZone: 'America/Los_Angeles',
        year: 'numeric', month: '2-digit', day: '2-digit',
        hour: '2-digit', minute: '2-digit', hour12: true
      });
      document.querySelectorAll('time.dt[data-epoch]').forEach(function(t){
        var sec = Number(t.getAttribute('data-epoch'));
        if (!isNaN(sec)) {
          var d = new Date(sec * 1000);
          t.textContent = fmt.format(d);
        }
      });
    })();
    </script>
)";

    printTail("content");
}
