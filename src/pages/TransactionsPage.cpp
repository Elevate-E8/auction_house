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
    sendHTMLHeader();
    printHead("My Transactions · Team Elevate", "content");

    // Ensure user is logged in
    if (!session_.isLoggedIn()) {
        std::cout << "<meta http-equiv='refresh' content='0;url=login.cgi'>\n";
        printTail("content");
        return;
    }

    MYSQL* conn = db_.connection();
    if (!conn) {
        std::cout << "<div class='error'>Database connection failed.</div>\n";
        printTail("content");
        return;
    }

    long userId = session_.userId();
    std::string email = session_.userEmail();

    // ---------------------------------------------------------------------
    // Page header
    // ---------------------------------------------------------------------
    std::cout << R"(
    <section class="card">
      <h2 style="margin:0 0 8px">My Transactions</h2>
      <p class="muted">Your selling activity, purchases, current bids, and items you didn't win.</p>
    </section>

    <section class="layout" aria-label="Transactions">
)";

    // =====================================================
    // CURRENT BIDS
    // =====================================================
    std::cout << R"(
      <article class="card bids">
        <h3 style="margin-top:0">Current Bids</h3>
        <table class="cbids" aria-label="Current bids">
          <colgroup><col><col><col><col></colgroup>
          <thead><tr><th>Item</th><th>Highest Bid</th><th>Your Max</th><th>Action</th></tr></thead>
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
                std::cout << "<tr>\n"
                    << "  <td>" << title << "</td>\n"
                    << "  <td>$" << highest << "</td>\n"
                    << "  <td>$" << yourmax << "</td>\n"
                    << "  <td>\n"
                    << "    <form class='inline-form' action='bid.cgi' method='post'>\n"
                    << "      <input type='hidden' name='item_id' value='" << item_id << "'>\n"
                    << "      <input name='bid_amount' type='number' step='0.01' placeholder='Enter new max' required>\n"
                    << "      <button class='btn primary' type='submit'>Increase</button>\n"
                    << "    </form>\n"
                    << "  </td>\n"
                    << "</tr>\n";
            }, 4);
        if (!any) std::cout << "<tr><td colspan='4'>No active bids.</td></tr>\n";
    }
    std::cout << "</tbody></table></article>\n";

    // =====================================================
    // SELLING
    // =====================================================
    std::cout << R"(
      <article class="card sell">
        <h3 style="margin-top:0">Selling</h3>
        <table aria-label="Items you are selling">
          <thead><tr><th>Item</th><th>Status</th><th>Ends</th></tr></thead>
          <tbody>
    )";
    {
        const char* sql =
            "SELECT title, "
            "CASE WHEN end_time<NOW() THEN 'Closed' ELSE 'Active' END, "
            "DATE_FORMAT(end_time,'%Y-%m-%d %H:%i') "
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
    std::cout << "</tbody></table></article>\n";

    // =====================================================
    // PURCHASES
    // =====================================================
    std::cout << R"(
      <article class="card purch">
        <h3 style="margin-top:0">Purchases</h3>
        <table aria-label="Items you purchased">
          <thead><tr><th>Item</th><th>Winning Bid</th><th>Closed</th></tr></thead>
          <tbody>
    )";
    {
        const char* sql =
            "SELECT i.title, "
            "IFNULL(FORMAT((SELECT bid_amount FROM bids WHERE bid_id=i.winning_bid_id),2),'0.00'), "
            "DATE_FORMAT(i.end_time,'%Y-%m-%d %H:%i') "
            "FROM items i WHERE i.winner_id=? ORDER BY i.end_time DESC";
        MYSQL_BIND p[1]; memset(p, 0, sizeof(p));
        p[0].buffer_type = MYSQL_TYPE_LONG; p[0].buffer = &userId; p[0].is_unsigned = 1;

        bool any = runQueryAndPrint(conn, sql, p, 1,
            [&](MYSQL_BIND* res, unsigned long*) {
                std::string title(htmlEscape((char*)res[0].buffer));
                std::string bid(htmlEscape((char*)res[1].buffer));
                std::string closed(htmlEscape((char*)res[2].buffer));
                std::cout << "<tr><td>" << title << "</td><td>$" << bid << "</td><td>" << closed << "</td></tr>\n";
            }, 3);
        if (!any) std::cout << "<tr><td colspan='3'>No purchases yet.</td></tr>\n";
    }
    std::cout << "</tbody></table></article>\n";

    // =====================================================
    // LOST
    // =====================================================
    std::cout << R"(
      <article class="card lost">
        <h3 style="margin-top:0">Didn't Win</h3>
        <table aria-label="Auctions you didn't win">
          <thead><tr><th>Item</th><th>Winning Bid</th><th>Closed</th></tr></thead>
          <tbody>
    )";
    {
        const char* sql =
            "SELECT i.title, "
            "IFNULL(FORMAT((SELECT bid_amount FROM bids WHERE bid_id=i.winning_bid_id),2),'0.00'), "
            "DATE_FORMAT(i.end_time,'%Y-%m-%d %H:%i') "
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
                std::string closed(htmlEscape((char*)res[2].buffer));
                std::cout << "<tr><td>" << title << "</td><td>$" << bid << "</td><td>" << closed << "</td></tr>\n";
            }, 3);
        if (!any) std::cout << "<tr><td colspan='3'>No lost auctions.</td></tr>\n";
    }
    std::cout << "</tbody></table></article>\n";

    // =====================================================
    // End page
    // =====================================================
    std::cout << "</section>\n";

    printTail("content");
}
