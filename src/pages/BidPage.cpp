// pages/BidPage.cpp
#include "pages/BidPage.hpp"
#include "utils/utils.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <cstdio>

BidPage::BidPage(Database& db, Session& session)
    : Page(db, session) {}


// -------------------------------------------------------------
// Helper: fetch active items, excluding the current user's own
// -------------------------------------------------------------

std::vector<BidPage::ItemOption> BidPage::fetchActiveItemsExcludingSeller(long excludeSellerId) {
    std::vector<ItemOption> items;
    MYSQL* conn = db_.connection();
    if (!conn) return items;

    const char* sql =
        "SELECT i.item_id, i.title "
        "FROM items i "
        "WHERE i.start_time <= NOW() "
        "  AND i.end_time > NOW() "
        "  AND (? <= 0 OR i.seller_id <> ?) "
        "ORDER BY i.end_time ASC, i.title ASC";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return items;

    if (mysql_stmt_prepare(stmt, sql, std::strlen(sql)) != 0) {
        mysql_stmt_close(stmt);
        return items;
    }

    MYSQL_BIND p[2]; std::memset(p, 0, sizeof(p));
    long ex = excludeSellerId;
    p[0].buffer_type = MYSQL_TYPE_LONG; p[0].buffer = &ex; p[0].is_unsigned = 1;
    p[1].buffer_type = MYSQL_TYPE_LONG; p[1].buffer = &ex; p[1].is_unsigned = 1;

    mysql_stmt_bind_param(stmt, p);
    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return items;
    }

    long idBuf = 0;
    char titleBuf[256]{};
    unsigned long titleLen = 0;

    MYSQL_BIND r[2]; std::memset(r, 0, sizeof(r));
    r[0].buffer_type = MYSQL_TYPE_LONG;   r[0].buffer = &idBuf;   r[0].is_unsigned = 1;
    r[1].buffer_type = MYSQL_TYPE_STRING; r[1].buffer = titleBuf; r[1].buffer_length = sizeof(titleBuf); r[1].length = &titleLen;

    mysql_stmt_bind_result(stmt, r);
    while (mysql_stmt_fetch(stmt) == 0) {
        ItemOption opt; opt.id = idBuf; opt.title.assign(titleBuf, titleLen);
        items.push_back(opt);
    }
    mysql_stmt_close(stmt);
    return items;
}

// -------------------------------------------------------------
// Helper: load seller, start price, current max bid, active flag
// -------------------------------------------------------------
bool BidPage::loadItemAndState(long itemId,
    long& sellerId,
    double& startPrice,
    double& currentMaxBid,
    bool& isActive) {
    MYSQL* conn = db_.connection();
    if (!conn) return false;

    const char* sql =
        "SELECT i.seller_id, i.start_price, "
        "       IFNULL((SELECT MAX(b.bid_amount) FROM bids b WHERE b.item_id=i.item_id), 0) AS max_bid, "
        "       (NOW() BETWEEN i.start_time AND i.end_time) AS is_active "
        "FROM items i WHERE i.item_id=? LIMIT 1";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return false;

    if (mysql_stmt_prepare(stmt, sql, std::strlen(sql)) != 0) {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND p{}; std::memset(&p, 0, sizeof(p));
    p.buffer_type = MYSQL_TYPE_LONG; p.buffer = &itemId; p.is_unsigned = 1;
    mysql_stmt_bind_param(stmt, &p);

    long seller = 0; double start = 0, maxBid = 0; int active = 0;

    MYSQL_BIND r[4]; std::memset(r, 0, sizeof(r));
    r[0].buffer_type = MYSQL_TYPE_LONG;   r[0].buffer = &seller; r[0].is_unsigned = 1;
    r[1].buffer_type = MYSQL_TYPE_DOUBLE; r[1].buffer = &start;
    r[2].buffer_type = MYSQL_TYPE_DOUBLE; r[2].buffer = &maxBid;
    r[3].buffer_type = MYSQL_TYPE_LONG;   r[3].buffer = &active;

    mysql_stmt_bind_result(stmt, r);
    bool ok = (mysql_stmt_execute(stmt) == 0) && (mysql_stmt_fetch(stmt) == 0);
    mysql_stmt_close(stmt);
    if (!ok) return false;

    sellerId = seller;
    startPrice = start;
    currentMaxBid = maxBid;
    isActive = (active != 0);
    return true;
}






// -------------------------------------------------------------
// GET — render form (excludes user’s own items if logged in)
// -------------------------------------------------------------
void BidPage::handleGet() {
    long exclude = session_.validate() ? session_.userId() : 0;
    auto items = fetchActiveItemsExcludingSeller(exclude);
    renderForm(items);
}

// -------------------------------------------------------------
// POST — validate and insert bid
// -------------------------------------------------------------
void BidPage::handlePost() {
    // Page::run() has already filled postData_ for POST.
    if (!session_.validate()) {
        auto items = fetchActiveItemsExcludingSeller(0);
        renderForm(items, "You must be logged in to place a bid.");
        return;
    }

    long userId = session_.userId();

    std::string itemIdStr = postData_["item_id"];
    std::string amountStr = postData_["bid_amount"];
    long itemId = 0;
    double amount = 0.0;

    // Basic input validation
    char* endp = nullptr;
    if (itemIdStr.empty() || (itemId = std::strtol(itemIdStr.c_str(), &endp, 10)) <= 0 || *endp != '\0') {
        auto items = fetchActiveItemsExcludingSeller(userId);
        renderForm(items, "Please select a valid item.", "", 0, amountStr);
        return;
    }
    try {
        size_t pos = 0;
        amount = std::stod(amountStr, &pos);
        if (pos != amountStr.size() || amount <= 0.0) throw std::runtime_error("bad");
    }
    catch (...) {
        auto items = fetchActiveItemsExcludingSeller(userId);
        renderForm(items, "Enter a valid positive bid amount.", "", itemId, amountStr);
        return;
    }

    // Load item state
    long sellerId = 0; double startPrice = 0.0; double currentMax = 0.0; bool isActive = false;
    if (!loadItemAndState(itemId, sellerId, startPrice, currentMax, isActive)) {
        auto items = fetchActiveItemsExcludingSeller(userId);
        renderForm(items, "Selected item does not exist.", "", 0, amountStr);
        return;
    }

    // Business rules
    if (sellerId == userId) {
        auto items = fetchActiveItemsExcludingSeller(userId);
        renderForm(items, "You cannot bid on your own item.", "", itemId, amountStr);
        return;
    }
    if (!isActive) {
        auto items = fetchActiveItemsExcludingSeller(userId);
        renderForm(items, "This auction is not currently active.", "", itemId, amountStr);
        return;
    }

    double floor = (currentMax > startPrice ? currentMax : startPrice);
    if (amount <= floor) {
        char msg[160];
        std::snprintf(msg, sizeof(msg),
            "Your bid must be greater than the current highest bid (or start price): $%.2f.",
            floor);
        auto items = fetchActiveItemsExcludingSeller(userId);
        renderForm(items, msg, "", itemId, amountStr);
        return;
    }

    // Insert bid
    // use existing DB connection
    MYSQL* conn = db_.connection();
    if (!conn) {
        auto items = fetchActiveItemsExcludingSeller(userId);
        renderForm(items, "Database connection failed. Please try again.", "", itemId, amountStr);
        return;
    }

    const char* sql =
        "INSERT INTO bids (item_id, bidder_id, bid_amount, bid_time) "
        "VALUES (?, ?, ?, NOW())";
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt || mysql_stmt_prepare(stmt, sql, std::strlen(sql)) != 0) {
        if (stmt) mysql_stmt_close(stmt);
        auto items = fetchActiveItemsExcludingSeller(userId);
        renderForm(items, "Internal error. Please try again.", "", itemId, amountStr);
        return;
    }

    MYSQL_BIND bp[3]; std::memset(bp, 0, sizeof(bp));
    bp[0].buffer_type = MYSQL_TYPE_LONG;   bp[0].buffer = &itemId; bp[0].is_unsigned = 1;
    bp[1].buffer_type = MYSQL_TYPE_LONG;   bp[1].buffer = &userId; bp[1].is_unsigned = 1;
    bp[2].buffer_type = MYSQL_TYPE_DOUBLE; bp[2].buffer = &amount;

    if (mysql_stmt_bind_param(stmt, bp) != 0 || mysql_stmt_execute(stmt) != 0) {
        std::string err = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        auto items = fetchActiveItemsExcludingSeller(userId);
        renderForm(items, "Failed to place bid: " + err, "", itemId, amountStr);
        return;
    }
    mysql_stmt_close(stmt);

    // (Optional) You could refresh items and show success
    auto items = fetchActiveItemsExcludingSeller(userId);
    char ok[128]; std::snprintf(ok, sizeof(ok), "Your bid of $%.2f has been placed.", amount);
    renderForm(items, "", ok);
}


// -------------------------------------------------------------
// Shared renderer
// -------------------------------------------------------------
void BidPage::renderForm(const std::vector<ItemOption>& items,
    const std::string& flashError,
    const std::string& flashSuccess,
    long selectedItemId,
    const std::string& enteredAmount) {
    sendHTMLHeader();
    printHead("Place a Bid · Team Elevate Auctions");

    const bool loggedIn = session_.validate();
    if (!loggedIn) {
        std::cout << R"(<div class="error" role="alert">
          You must be logged in to place a bid.
          <a href="login.cgi">Log in</a> or <a href="register.cgi">create an account</a>.
        </div>)";
    }
    else {
        std::cout << "  <div class='success' role='status'>Logged in as <strong>"
            << htmlEscape(session_.userEmail()) << "</strong></div>\n";
    }

    if (!flashError.empty()) {
        std::cout << "  <div class='error' role='alert'>"
            << htmlEscape(flashError) << "</div>\n";
    }
    if (!flashSuccess.empty()) {
        std::cout << "  <div class='success' role='status'>"
            << htmlEscape(flashSuccess) << "</div>\n";
    }

    std::cout << R"(
<section class="card" aria-labelledby="bid-heading">
  <h2 id="bid-heading" style="margin-top:0;">Bid on an Item</h2>
  <p class="helper">
    Choose an active listing and enter your highest bid.
    You cannot bid on your own items.
  </p>
)";

    if (items.empty()) {
        std::cout << R"(
  <div class="muted">
    There are no eligible items available to bid on right now.
  </div>
</section>
)";
        printTail();
        return;
    }

    std::cout << R"(
  <form method="post" action="bid.cgi" novalidate>
    <label for="itemSelect">Item</label>
    <select id="itemSelect" name="item_id" required
            style="width:100%; padding:12px 14px; border:1px solid var(--border);
                   border-radius:12px; font-size:15px; background:#fff;">
      <option value="">Select an item…</option>
)";

    for (const auto& it : items) {
        std::cout << "      <option value='" << it.id << "'";
        if (selectedItemId == it.id) {
            std::cout << " selected";
        }
        std::cout << ">" << htmlEscape(it.title) << "</option>\n";
    }

    std::cout << R"(
    </select>

    <label for="bidAmount" style="margin-top:12px;">Your highest bid</label>
    <input id="bidAmount" name="bid_amount" type="number" inputmode="decimal"
           step="0.01" min="0.01" placeholder="0.00" required
           value=")";

    // fill in prior input safely
    std::cout << htmlEscape(enteredAmount) << R"(" />

    <div style="display:flex; gap:10px; margin-top:16px;">
)";

    // Submit button (disable if not logged in)
    std::cout << "      <button class='btn primary' type='submit'";
    if (!loggedIn) {
        std::cout << " disabled";
    }
    std::cout << ">Place Bid</button>\n";

    // Cancel link
    std::cout << "      <a class='btn' href='index.cgi' "
        "style='border:1px solid var(--border); background:#fff;'>Cancel</a>\n";
    std::cout << "    </div>\n";

    std::cout << R"(
  </form>
</section>
)";
    printTail();
}
