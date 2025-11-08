#include "pages/SellPage.hpp"
#include "utils/utils.hpp"
#include <iostream>
#include <cstring>
#include <ctime>

SellPage::SellPage(Database& db, Session& session)
    : Page(db, session) {
}

void SellPage::handleGet() {
    sendHTMLHeader();
    printHead("Sell an Item · Team Elevate Auctions");

    const bool isLoggedIn = session_.validate();
    const std::string userEmail = isLoggedIn ? session_.userEmail() : "";

    if (isLoggedIn) {
        std::cout << "  <div class='success' role='status'>"
                  << "Logged in as <strong>" << htmlEscape(userEmail) << "</strong>"
                  << "</div>\n";
    } else {
        std::cout << R"(  <div class="error" role="alert">
      You must be logged in to list an item. <a href="login.cgi">Log in</a> or <a href="register.cgi">create an account</a>.
    </div>
)";
    }

    std::cout << R"(
    <section class="card" aria-labelledby="sell-heading">
      <h2 id="sell-heading" style="margin-top:0;">Sell an Item</h2>
      <p class="helper">All auctions run for <strong>7 days</strong> from the start date &amp; time.</p>

      <div id="sell-success" class="success" style="display:none;">Your item has been listed.</div>
      <div id="sell-error" class="error" style="display:none;">Please fix the errors below and try again.</div>

      <form method="post" action="sell.cgi" novalidate>
        <!-- Item name -->
        <label for="itemName">Item name</label>
        <input id="itemName" name="item_name" type="text" maxlength="120"
               placeholder="e.g., Nintendo Switch OLED, 64GB" required />

        <!-- Condition -->
        <label for="cond">Condition</label>
        <select id="cond" name="condition" required
                style="width:100%; padding:12px 14px; border:1px solid var(--border); border-radius:12px; font-size:15px; background:#fff;">
          <option value="" disabled selected>Select condition…</option>
          <option value="new">New</option>
          <option value="like_new">Like New</option>
          <option value="good">Good</option>
          <option value="fair">Fair</option>
          <option value="poor">Poor</option>
        </select>
        <ol class="helper" style="margin:6px 0 12px 18px;">
          <li><strong>New</strong> — Unused and unopened in original packaging.</li>
          <li><strong>Like New</strong> — Lightly used with no visible damage.</li>
          <li><strong>Good</strong> — Gently used with minor flaws.</li>
          <li><strong>Fair</strong> — Well-used with visible signs of wear.</li>
          <li><strong>Poor</strong> — Heavily worn, may only be usable for parts.</li>
        </ol>

        <!-- Description -->
        <label for="desc">Description of item</label>
        <textarea id="desc" name="description" rows="6" required
          style="width:100%; padding:12px 14px; border:1px solid var(--border); border-radius:12px; font-size:15px; background:#fff;"></textarea>
        <p class="helper">Be specific: exact model, size/dimensions, accessories, and notable flaws.</p>

        <!-- Starting price -->
        <label for="startPrice">Starting bid price</label>
        <input id="startPrice" name="starting_price" type="number" inputmode="decimal" step="0.01" min="0.01" placeholder="0.00" required />
        <p class="helper">Use cents if needed (e.g., 19.99).</p>

        <!-- Start date/time -->
        <label for="startAt">Starting date &amp; time</label>
        <input id="startAt" name="start_datetime" type="datetime-local" required />
        <p class="helper">The auction ends exactly <strong>7 days</strong> after this start time.</p>

        <div id="endReadout" class="muted" style="margin-top:4px;"></div>

        <div style="display:flex; gap:10px; margin-top:12px;">
          <button class="btn primary" type="submit">List Item</button>
          <a class="btn" href="index.cgi" style="border:1px solid var(--border); background:#fff;">Cancel</a>
        </div>
      </form>
    </section>

    <script>
    (function () {
      var start = document.getElementById('startAt');
      var readout = document.getElementById('endReadout');
      function pad(n){ return (n < 10 ? '0' : '') + n; }
      function fmt(dt){
        var month = pad(dt.getMonth()+1);
        var day = pad(dt.getDate());
        var year = dt.getFullYear();
        var hour = dt.getHours();
        var minute = pad(dt.getMinutes());
        var ampm = hour >= 12 ? ' PM' : ' AM';
        if (hour > 12) hour -= 12;
        if (hour === 0) hour = 12;
        return month + '/' + day + '/' + year + ' ' + hour + ':' + minute + ampm;
    }
      function update(){
        if (!start || !start.value) { readout.textContent = ''; return; }
        var s = new Date(start.value);
        if (isNaN(s.getTime())) { readout.textContent = ''; return; }
        // 168 hours = 7 days
        var end = new Date(s.getTime() + 168 * 60 * 60 * 1000);
        readout.textContent = 'Ends: ' + fmt(end) + ' (7 days after start)';
      }
      if (start) { start.addEventListener('input', update); update(); }
    })();
    </script>
)";

    printTail();
}

void SellPage::handlePost() {
    // Check if user is logged in
    if (!session_.validate()) {
        sendHTMLHeader();
        printHead("Sell an Item · Team Elevate Auctions");
        std::cout << R"(  <div class="error" role="alert">
      You must be logged in to list an item. <a href="login.cgi">Log in</a> or <a href="register.cgi">create an account</a>.
    </div>
)";
        printTail();
        return;
    }

    // Get form data
    std::string itemName = postData_["item_name"];
    std::string condition = postData_["condition"];
    std::string description = postData_["description"];
    std::string startingPriceStr = postData_["starting_price"];
    std::string startDatetime = postData_["start_datetime"];

    // Helper function to show form with error
    auto showFormWithError = [&](const std::string& errorMsg) {
        sendHTMLHeader();
        printHead("Sell an Item · Team Elevate Auctions");

        std::cout << "  <div class='success' role='status'>"
                  << "Logged in as <strong>" << htmlEscape(session_.userEmail()) << "</strong>"
                  << "</div>\n";

        std::cout << "  <div class='error' role='alert'>" << htmlEscape(errorMsg) << "</div>\n";

        // Re-render form with preserved values
        std::cout << R"(
    <section class="card" aria-labelledby="sell-heading">
      <h2 id="sell-heading" style="margin-top:0;">Sell an Item</h2>
      <p class="helper">All auctions run for <strong>7 days</strong> from the start date &amp; time.</p>

      <form method="post" action="sell.cgi" novalidate>
        <label for="itemName">Item name</label>
        <input id="itemName" name="item_name" type="text" maxlength="120"
               placeholder="e.g., Nintendo Switch OLED, 64GB" required
               value=")" << htmlEscape(itemName) << R"(" />

        <label for="cond">Condition</label>
        <select id="cond" name="condition" required
                style="width:100%; padding:12px 14px; border:1px solid var(--border); border-radius:12px; font-size:15px; background:#fff;">
          <option value="" disabled)";
        if (condition.empty()) std::cout << " selected";
        std::cout << R"(>Select condition…</option>
          <option value="new")";
        if (condition == "new") std::cout << " selected";
        std::cout << R"(>New</option>
          <option value="like_new")";
        if (condition == "like_new") std::cout << " selected";
        std::cout << R"(>Like New</option>
          <option value="good")";
        if (condition == "good") std::cout << " selected";
        std::cout << R"(>Good</option>
          <option value="fair")";
        if (condition == "fair") std::cout << " selected";
        std::cout << R"(>Fair</option>
          <option value="poor")";
        if (condition == "poor") std::cout << " selected";
        std::cout << R"(>Poor</option>
        </select>

        <label for="desc">Description of item</label>
        <textarea id="desc" name="description" rows="6" required
          style="width:100%; padding:12px 14px; border:1px solid var(--border); border-radius:12px; font-size:15px; background:#fff;">)"
          << htmlEscape(description) << R"(</textarea>

        <label for="startPrice">Starting bid price</label>
        <input id="startPrice" name="starting_price" type="number" inputmode="decimal" step="0.01" min="0.01"
               placeholder="0.00" required value=")" << htmlEscape(startingPriceStr) << R"(" />

        <label for="startAt">Starting date &amp; time</label>
        <input id="startAt" name="start_datetime" type="datetime-local" required
               value=")" << htmlEscape(startDatetime) << R"(" />

        <div style="display:flex; gap:10px; margin-top:12px;">
          <button class="btn primary" type="submit">List Item</button>
          <a class="btn" href="index.cgi" style="border:1px solid var(--border); background:#fff;">Cancel</a>
        </div>
      </form>
    </section>
)";
        printTail();
    };

    // Validate inputs
    if (itemName.empty() || itemName.length() > 100) {
        showFormWithError("Item name is required and must be 100 characters or less.");
        return;
    }

    if (condition.empty()) {
        showFormWithError("Please select a condition.");
        return;
    }

    if (description.empty()) {
        showFormWithError("Description is required.");
        return;
    }

    if (startingPriceStr.empty()) {
        showFormWithError("Starting price is required.");
        return;
    }

    // Parse and validate price
    double startingPrice = 0.0;
    try {
        size_t pos = 0;
        startingPrice = std::stod(startingPriceStr, &pos);

        // Check if the entire string was consumed (valid number)
        if (pos != startingPriceStr.length()) {
            showFormWithError("Invalid starting price format.");
            return;
        }

        if (startingPrice <= 0) {
            showFormWithError("Starting price must be greater than zero.");
            return;
        }
    } catch (...) {
        showFormWithError("Invalid starting price format.");
        return;
    }

    if (startDatetime.empty() || startDatetime.length() < 16) {
        showFormWithError("Starting date and time is required.");
        return;
    }

    // Basic format check for datetime (should be like "2025-11-07T10:00")
    if (startDatetime.find('T') == std::string::npos && startDatetime.find(' ') == std::string::npos) {
        showFormWithError("Invalid date/time format.");
        return;
    }

    // Parse datetime (format: YYYY-MM-DDTHH:MM)
    // Convert to MySQL datetime format: YYYY-MM-DD HH:MM:SS
    std::string startTimeMysql = startDatetime;
    size_t tPos = startTimeMysql.find('T');
    if (tPos != std::string::npos) {
        startTimeMysql[tPos] = ' ';
    }
    startTimeMysql += ":00"; // Add seconds

    // Get database connection
    MYSQL* conn = db_.connection();
    if (!conn) {
        showFormWithError("Database connection failed. Please try again later.");
        return;
    }

    // Validate that start time is not in the past
    const char* checkSql = "SELECT ? > NOW() as is_future";
    MYSQL_STMT* checkStmt = mysql_stmt_init(conn);
    if (checkStmt && mysql_stmt_prepare(checkStmt, checkSql, std::strlen(checkSql)) == 0) {
        MYSQL_BIND checkParam{};
        std::memset(&checkParam, 0, sizeof(checkParam));
        checkParam.buffer_type = MYSQL_TYPE_STRING;
        checkParam.buffer = (char*)startTimeMysql.c_str();
        checkParam.buffer_length = startTimeMysql.size();

        mysql_stmt_bind_param(checkStmt, &checkParam);
        mysql_stmt_execute(checkStmt);

        MYSQL_BIND checkResult{};
        int isFuture = 0;
        checkResult.buffer_type = MYSQL_TYPE_LONG;
        checkResult.buffer = &isFuture;
        mysql_stmt_bind_result(checkStmt, &checkResult);
        mysql_stmt_fetch(checkStmt);
        mysql_stmt_close(checkStmt);

        if (isFuture == 0) {
            showFormWithError("Starting date and time must be in the future.");
            return;
        }
    } else {
        if (checkStmt) mysql_stmt_close(checkStmt);
        showFormWithError("Failed to validate start time.");
        return;
    }

    // Insert the item into the database
    const char* sql =
        "INSERT INTO items (seller_id, title, description, start_price, start_time, end_time) "
        "VALUES (?, ?, ?, ?, ?, DATE_ADD(?, INTERVAL 7 DAY))";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) {
        showFormWithError("Internal server error.");
        return;
    }

    if (mysql_stmt_prepare(stmt, sql, std::strlen(sql)) != 0) {
        mysql_stmt_close(stmt);
        showFormWithError("Internal server error.");
        return;
    }

    // Bind parameters
    MYSQL_BIND params[6];
    std::memset(params, 0, sizeof(params));

    long sellerId = session_.userId();

    // seller_id
    params[0].buffer_type = MYSQL_TYPE_LONG;
    params[0].buffer = &sellerId;
    params[0].is_unsigned = 1;

    // title
    params[1].buffer_type = MYSQL_TYPE_STRING;
    params[1].buffer = (char*)itemName.c_str();
    params[1].buffer_length = itemName.size();

    // description
    params[2].buffer_type = MYSQL_TYPE_STRING;
    params[2].buffer = (char*)description.c_str();
    params[2].buffer_length = description.size();

    // start_price
    params[3].buffer_type = MYSQL_TYPE_DOUBLE;
    params[3].buffer = &startingPrice;

    // start_time
    params[4].buffer_type = MYSQL_TYPE_STRING;
    params[4].buffer = (char*)startTimeMysql.c_str();
    params[4].buffer_length = startTimeMysql.size();

    // end_time (for DATE_ADD calculation)
    params[5].buffer_type = MYSQL_TYPE_STRING;
    params[5].buffer = (char*)startTimeMysql.c_str();
    params[5].buffer_length = startTimeMysql.size();

    if (mysql_stmt_bind_param(stmt, params) != 0) {
        mysql_stmt_close(stmt);
        showFormWithError("Internal server error.");
        return;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        std::string error = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        showFormWithError("Failed to list item: " + error);
        return;
    }

    mysql_stmt_close(stmt);

    // Success! Show confirmation page
    sendHTMLHeader();
    printHead("Item Listed Successfully · Team Elevate Auctions");

    // Format price to 2 decimal places
    char priceBuffer[32];
    std::snprintf(priceBuffer, sizeof(priceBuffer), "%.2f", startingPrice);

    // Parse and format datetime to MM/DD/YYYY h:MM AM/PM
    std::string displayDatetime = startDatetime;
    size_t tDisplay = displayDatetime.find('T');
    if (tDisplay != std::string::npos) {
       displayDatetime[tDisplay] = ' ';
    }

    // Parse YYYY-MM-DD HH:MM format
    if (displayDatetime.length() >= 16) {
        std::string year = displayDatetime.substr(0, 4);
        std::string month = displayDatetime.substr(5, 2);
        std::string day = displayDatetime.substr(8, 2);
        std::string hourStr = displayDatetime.substr(11, 2);
        std::string minute = displayDatetime.substr(14, 2);

        int hour = std::stoi(hourStr);
        std::string ampm = (hour >= 12) ? " PM" : " AM";
        if (hour > 12) hour -= 12;
        if (hour == 0) hour = 12;

    // Format as MM/DD/YYYY h:MM AM/PM
    displayDatetime = month + "/" + day + "/" + year + " "
                    + std::to_string(hour) + ":" + minute + ampm;
    }


    std::cout << R"(
    <section class="card" role="status" aria-live="polite">
      <h1>✓ Item Listed Successfully</h1>
      <div class="success">Your item "<strong>)" << htmlEscape(itemName) << R"(</strong>" has been listed for auction.</div>
      <div class="muted" style="margin-top:12px;">
        <strong>Starting price:</strong> $)" << priceBuffer << R"(<br>
        <strong>Auction starts:</strong> )" << htmlEscape(displayDatetime) << R"(<br>
        <strong>Duration:</strong> 7 days
      </div>
      <p class="muted">You'll be redirected to your transactions page…</p>
      <meta http-equiv="refresh" content="3;url=transactions.cgi">
      <div style="margin-top:16px; display:flex; gap:10px;">
        <a class="btn primary" href="transactions.cgi">View My Transactions</a>
        <a class="btn" href="sell.cgi" style="border:1px solid var(--border); background:#fff;">List Another Item</a>
      </div>
    </section>
)";

    printTail();
}
