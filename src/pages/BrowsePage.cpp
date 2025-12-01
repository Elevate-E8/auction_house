#include "pages/BrowsePage.hpp"
#include "utils/utils.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <cstring>
#include <cstdlib>

// -------------------------------------------------------------
// Helper: parse QUERY_STRING into q (search) and sort
// Uses urlDecode from utils.hpp
// -------------------------------------------------------------
static void parseQueryString(std::string& qParam, std::string& sortParam) {
    const char* env = std::getenv("QUERY_STRING");
    if (!env) {
        return;
    }

    std::string qs(env);
    std::size_t pos = 0;

    while (pos < qs.size()) {
        std::size_t amp = qs.find('&', pos);
        std::string pair;
        if (amp == std::string::npos) {
            pair = qs.substr(pos);
        }
        else {
            pair = qs.substr(pos, amp - pos);
        }

        std::size_t eq = pair.find('=');
        std::string key;
        std::string value;
        if (eq != std::string::npos) {
            key = pair.substr(0, eq);
            value = pair.substr(eq + 1);
        }
        else {
            key = pair;
            value = "";
        }

        key = urlDecode(key);
        value = urlDecode(value);

        if (key == "q") {
            qParam = value;
        }
        else if (key == "sort") {
            sortParam = value;
        }

        if (amp == std::string::npos) {
            pos = qs.size();
        }
        else {
            pos = amp + 1;
        }
    }
}

// -------------------------------------------------------------
// Helper: format currency as $12.34
// -------------------------------------------------------------
static std::string formatCurrency(double amount) {
    std::ostringstream oss;
    oss.setf(std::ios::fixed, std::ios::floatfield);
    oss << std::setprecision(2) << amount;
    std::string value = oss.str();
    return std::string("$") + value;
}

// -------------------------------------------------------------
// Helper: format remaining time until end_time as a short string
// Uses server local time via std::time / std::mktime.
// -------------------------------------------------------------
static std::string formatTimeLeft(const MYSQL_TIME& endTime) {
    std::tm tmEnd;
    std::memset(&tmEnd, 0, sizeof(tmEnd));

    tmEnd.tm_year = static_cast<int>(endTime.year) - 1900;
    tmEnd.tm_mon = static_cast<int>(endTime.month) - 1;
    tmEnd.tm_mday = static_cast<int>(endTime.day);
    tmEnd.tm_hour = static_cast<int>(endTime.hour);
    tmEnd.tm_min = static_cast<int>(endTime.minute);
    tmEnd.tm_sec = static_cast<int>(endTime.second);

    std::time_t endT = std::mktime(&tmEnd);
    std::time_t nowT = std::time(NULL);

    if (endT <= nowT) {
        return "Ended";
    }

    long diff = static_cast<long>(endT - nowT);
    long days = diff / 86400;
    diff = diff % 86400;
    long hours = diff / 3600;
    diff = diff % 3600;
    long minutes = diff / 60;
    long seconds = diff % 60;

    std::ostringstream oss;

    if (days > 0) {
        // Example: "2d 03h"
        oss << days << "d ";
        if (hours < 10) {
            oss << "0";
        }
        oss << hours << "h";
    }
    else {
        // Example: "03:14:07"
        if (hours < 10) {
            oss << "0";
        }
        oss << hours << ":";
        if (minutes < 10) {
            oss << "0";
        }
        oss << minutes << ":";
        if (seconds < 10) {
            oss << "0";
        }
        oss << seconds;
    }

    return oss.str();
}

// -------------------------------------------------------------
// BrowsePage
// -------------------------------------------------------------
BrowsePage::BrowsePage(Database& db, Session& session)
    : Page(db, session) {
}

// -------------------------------------------------------------
// GET — show all unexpired auctions with optional search/sort
// -------------------------------------------------------------
void BrowsePage::handleGet() {
    sendHTMLHeader();
    printHead("Browse Auctions · Team Elevate Auctions");

    bool isLoggedIn = session_.isLoggedIn();
    long currentUserId = session_.userId();

    // ---------------------------------------------------------
    // Read query parameters q (search) and sort
    // ---------------------------------------------------------
    std::string qParam;
    std::string sortParam;
    parseQueryString(qParam, sortParam);

    // Normalize sort key
    std::string sortKey = "ending";
    if (sortParam == "newest") {
        sortKey = "newest";
    }
    else if (sortParam == "low") {
        sortKey = "low";
    }
    else if (sortParam == "high") {
        sortKey = "high";
    }

    std::string searchTerm = qParam;
    std::string searchEscaped = htmlEscape(searchTerm);

    std::string selEnding = "";
    std::string selNewest = "";
    std::string selLow = "";
    std::string selHigh = "";

    if (sortKey == "ending") {
        selEnding = " selected";
    }
    else if (sortKey == "newest") {
        selNewest = " selected";
    }
    else if (sortKey == "low") {
        selLow = " selected";
    }
    else if (sortKey == "high") {
        selHigh = " selected";
    }

    // ---------------------------------------------------------
    // Render top of page, including search/sort form
    // ---------------------------------------------------------
    std::cout
        << "<section class=\"card\" aria-labelledby=\"browse-heading\">\n"
        << "  <h2 id=\"browse-heading\" style=\"margin-top:0\">Browse Auctions</h2>\n\n"
        << "  <!-- Top controls: search + sort (submit via GET) -->\n"
        << "  <form method=\"get\" action=\"browse.cgi\" style=\"margin-bottom:12px;\">\n"
        << "    <div style=\"display:flex; gap:12px; flex-wrap:wrap;\">\n"
        << "      <input type=\"search\" name=\"q\""
        << " placeholder=\"Search items...\" aria-label=\"Search items\""
        << " value=\"" << searchEscaped << "\""
        << " style=\"flex:1; min-width:220px; padding:10px 12px; border:1px solid var(--border);"
        << " border-radius:12px; font-size:14px; background:#fff;\">\n"
        << "      <select name=\"sort\" aria-label=\"Sort\""
        << " style=\"padding:10px 12px; border:1px solid var(--border);"
        << " border-radius:12px; background:#fff; min-width:180px;\">\n"
        << "        <option value=\"ending\"" << selEnding
        << ">Sort: Ending soon</option>\n"
        << "        <option value=\"newest\"" << selNewest
        << ">Sort: Newest</option>\n"
        << "        <option value=\"low\"" << selLow
        << ">Sort: Lowest price</option>\n"
        << "        <option value=\"high\"" << selHigh
        << ">Sort: Highest price</option>\n"
        << "      </select>\n"
        << "      <button type=\"submit\" class=\"btn ghost\">Apply</button>\n"
        << "    </div>\n"
        << "  </form>\n\n"
        << "  <!-- Scrollable list area -->\n"
        << "  <div class=\"table-wrap\" style=\"\n"
        << "        border:1px solid var(--border);\n"
        << "        border-radius:12px;\n"
        << "        overflow:hidden;\n"
        << "        background:#fff;\n"
        << "      \">\n"
        << "    <table aria-label=\"Auction items list\" style=\"margin:0;\">\n"
        << "      <colgroup>\n"
        << "        <col style=\"width:35%;\"> <!-- Item -->\n"
        << "        <col style=\"width:30%;\"> <!-- Seller -->\n"
        << "        <col style=\"width:20%;\"> <!-- Current Bid -->\n"
        << "        <col style=\"width:15%;\"> <!-- Time Left -->\n"
        << "      </colgroup>\n"
        << "      <thead>\n"
        << "        <tr>\n"
        << "          <th scope=\"col\">Item</th>\n"
        << "          <th scope=\"col\">Seller</th>\n"
        << "          <th scope=\"col\">Current&nbsp;Bid</th>\n"
        << "          <th scope=\"col\">Time&nbsp;Left</th>\n"
        << "        </tr>\n"
        << "      </thead>\n"
        << "    </table>\n\n"
        << "    <!-- Body in its own scroll region so header stays fixed -->\n"
        << "    <div id=\"rowsViewport\" style=\"max-height:420px; overflow:auto; border-top:1px solid var(--border);\">\n"
        << "      <table style=\"margin:0; border-spacing:0;\">\n"
        << "        <colgroup>\n"
        << "          <col style=\"width:35%;\"> <!-- Item -->\n"
        << "          <col style=\"width:30%;\"> <!-- Seller -->\n"
        << "          <col style=\"width:20%;\"> <!-- Current Bid -->\n"
        << "          <col style=\"width:15%;\"> <!-- Time Left -->\n"
        << "        </colgroup>\n"
        << "        <tbody id=\"js-rows\">\n"
        << "          <!-- hidden placeholder row so <col> has cells; removed once real rows appear -->\n"
        << "          <tr id=\"placeholderRow\" style=\"visibility:collapse;\">\n"
        << "            <td></td><td></td><td></td><td></td>\n"
        << "          </tr>\n";

    // ---------------------------------------------------------
    // unexpired auctions with optional search/sort
    // ---------------------------------------------------------
    MYSQL* conn = db_.connection();
    if (conn) {
        // Base query
        std::string sql =
            "SELECT i.item_id, i.title, u.user_email, i.seller_id, "
            "       COALESCE(MAX(b.bid_amount), i.start_price) AS current_bid, "
            "       i.end_time "
            "FROM items i "
            "JOIN users u ON i.seller_id = u.user_id "
            "LEFT JOIN bids b ON b.item_id = i.item_id "
            "WHERE i.end_time > NOW()";

        bool hasSearch = (searchTerm.size() > 0);
        if (hasSearch) {
            sql += " AND (i.title LIKE ? OR i.description LIKE ?)";
        }

        sql += " GROUP BY i.item_id, i.title, u.user_email, i.seller_id, i.start_price, i.end_time ";

        if (sortKey == "newest") {
            sql += "ORDER BY i.start_time DESC";
        }
        else if (sortKey == "low") {
            sql += "ORDER BY current_bid ASC";
        }
        else if (sortKey == "high") {
            sql += "ORDER BY current_bid DESC";
        }
        else {
            sql += "ORDER BY i.end_time ASC";
        }

        MYSQL_STMT* stmt = mysql_stmt_init(conn);
        if (stmt && mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) == 0) {

            // Bind search parameters if needed
            MYSQL_BIND params[2];
            std::memset(params, 0, sizeof(params));
            std::string likePattern;
            if (hasSearch) {
                likePattern = "%" + searchTerm + "%";

                params[0].buffer_type = MYSQL_TYPE_STRING;
                params[0].buffer = (char*)likePattern.c_str();
                params[0].buffer_length = likePattern.size();

                params[1].buffer_type = MYSQL_TYPE_STRING;
                params[1].buffer = (char*)likePattern.c_str();
                params[1].buffer_length = likePattern.size();

                mysql_stmt_bind_param(stmt, params);
            }

            if (mysql_stmt_execute(stmt) == 0) {
                // Bind result columns
                int itemId = 0;
                long sellerId = 0;

                char titleBuf[101];
                unsigned long titleLen = 0;

                char emailBuf[129];
                unsigned long emailLen = 0;

                double currentBid = 0.0;
                MYSQL_TIME endTime;

                MYSQL_BIND result[6];
                std::memset(result, 0, sizeof(result));

                result[0].buffer_type = MYSQL_TYPE_LONG;
                result[0].buffer = &itemId;
                result[0].is_unsigned = 1;

                result[1].buffer_type = MYSQL_TYPE_STRING;
                result[1].buffer = titleBuf;
                result[1].buffer_length = sizeof(titleBuf);
                result[1].length = &titleLen;

                result[2].buffer_type = MYSQL_TYPE_STRING;
                result[2].buffer = emailBuf;
                result[2].buffer_length = sizeof(emailBuf);
                result[2].length = &emailLen;

                result[3].buffer_type = MYSQL_TYPE_LONG;
                result[3].buffer = &sellerId;
                result[3].is_unsigned = 1;

                result[4].buffer_type = MYSQL_TYPE_DOUBLE;
                result[4].buffer = &currentBid;

                result[5].buffer_type = MYSQL_TYPE_DATETIME;
                result[5].buffer = &endTime;

                if (mysql_stmt_bind_result(stmt, result) == 0) {
                    mysql_stmt_store_result(stmt);

                    int fetchStatus = mysql_stmt_fetch(stmt);
                    while (fetchStatus == 0 || fetchStatus == MYSQL_DATA_TRUNCATED) {
                        std::string title(titleBuf, titleLen);
                        std::string sellerEmail(emailBuf, emailLen);

                        std::string timeLeft = formatTimeLeft(endTime);
                        std::string bidText = formatCurrency(currentBid);

                        bool canBid =
                            isLoggedIn &&
                            currentUserId > 0 &&
                            currentUserId != sellerId;

                        std::cout << "          <tr>\n";

                        // Item: link to bid page for that item
                        std::cout << "            <td>";
                        std::cout << "<a href='bid.cgi?item_id="
                            << itemId
                            << "'>"
                            << htmlEscape(title)
                            << "</a>";
                        std::cout << "</td>\n";

                        // Seller
                        std::cout << "            <td>"
                            << htmlEscape(sellerEmail)
                            << "</td>\n";

                        // Current bid (+ optional Bid button)
                        std::cout << "            <td>"
                            << htmlEscape(bidText);
                        if (canBid) {
                            std::cout << "  ";
                            std::cout << "<a class='btn primary' "
                                "style='margin-left:6px; display:inline-block;"
                                "vertical-align:middle;' "
                                "href='bid.cgi?item_id="
                                << itemId
                                << "'>Bid</a>";
                        }

                        std::cout << "</td>\n";

                        // Time left
                        std::cout << "            <td>"
                            << htmlEscape(timeLeft)
                            << "</td>\n";

                        std::cout << "          </tr>\n";

                        fetchStatus = mysql_stmt_fetch(stmt);
                    }
                }
            }

            mysql_stmt_close(stmt);
        }
        else if (stmt) {
            mysql_stmt_close(stmt);
        }
    }
    else {
        // DB connection missing – show a single error row
        std::cout
            << "          <tr>\n"
            << "            <td colspan=\"4\" class=\"error\">"
            << "Unable to load auctions at this time."
            << "</td>\n"
            << "          </tr>\n";
    }

    // Close tbody/table and finish template + script
    std::cout
        << "        </tbody>\n"
        << "      </table>\n\n"
        << "      <!-- Empty state (shown only when there are no rows) -->\n"
        << "      <div id=\"emptyState\" style=\"padding:14px; color:var(--muted); display:none;\">\n"
        << "        No items to show yet.\n"
        << "      </div>\n"
        << "    </div>\n"
        << "  </div>\n\n"
        << "  <!-- No pagination controls; scrolling handles browsing all items. -->\n"
        << "</section>\n\n"
        << "<script>\n"
        << "(function () {\n"
        << "  var tbody = document.getElementById('js-rows');\n"
        << "  var emptyState = document.getElementById('emptyState');\n"
        << "  var placeholder = document.getElementById('placeholderRow');\n\n"
        << "  function updateEmptyState() {\n"
        << "    var rows = Array.prototype.filter.call(\n"
        << "      tbody.querySelectorAll('tr'),\n"
        << "      function (tr) { return tr !== placeholder; }\n"
        << "    );\n\n"
        << "    if (rows.length > 0 && placeholder && placeholder.parentNode) {\n"
        << "      placeholder.parentNode.removeChild(placeholder);\n"
        << "      placeholder = null;\n"
        << "    }\n\n"
        << "    emptyState.style.display = rows.length > 0 ? 'none' : 'block';\n"
        << "  }\n\n"
        << "  window.refreshBrowse = updateEmptyState;\n"
        << "  updateEmptyState();\n"
        << "})();\n"
        << "</script>\n";

    printTail();
}
