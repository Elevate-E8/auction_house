// pages/SellPage.cpp
#include "pages/SellPage.hpp"
#include "utils/utils.hpp"
#include <iostream>

SellPage::SellPage(Database& db, Session& session)
    : Page(db, session) {
}

void SellPage::handleGet() {
    sendHTMLHeader();                                       // CGI header
    printHead("Sell an Item · Team Elevate Auctions");      // shared header/nav

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
      <p class="helper">All auctions run for <strong>7 days</strong> from the start date &amp; time. Images are omitted for simplicity.</p>

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
        <input id="startPrice" name="starting_price" type="number" inputmode="decimal" step="0.01" min="0" placeholder="0.00" required />
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
        return dt.getFullYear() + '-' + pad(dt.getMonth()+1) + '-' + pad(dt.getDate())
             + ' ' + pad(dt.getHours()) + ':' + pad(dt.getMinutes());
      }
      function update(){
        if (!start || !start.value) { readout.textContent = ''; return; }
        var s = new Date(start.value);
        if (isNaN(s.getTime())) { readout.textContent = ''; return; }
        // Internally still 168 hours = 7 days
        var end = new Date(s.getTime() + 168 * 60 * 60 * 1000);
        readout.textContent = 'Ends: ' + fmt(end) + ' (7 days after start)';
      }
      if (start) { start.addEventListener('input', update); update(); }
    })();
    </script>
)";

    printTail();                                             // shared footer
}
