// pages/BidPage.cpp
#include "pages/BidPage.hpp"
#include "utils/utils.hpp"
#include <iostream>

BidPage::BidPage(Database& db, Session& session)
    : Page(db, session) {}

// -------------------------------------------------------------
// GET — Render the Bid page (purely presentational template)
// -------------------------------------------------------------
void BidPage::handleGet() {
    sendHTMLHeader();
    printHead("Bid on Item · Team Elevate Auctions");

    // NOTE TO BACKEND:
    // Replace these placeholders with real values from the DB/querystring.
    // Keep the same element IDs so the layout remains consistent.
    const std::string ITEM_NAME       = "ITEM_NAME";        // ex: "Vintage Camera"
    const std::string ITEM_CONDITION  = "";                 // ex: "Like New" (PILL REMOVED; leave empty or ignore)
    const std::string ITEM_DESC       = "DESCRIPTION";      // ex: long text
    const std::string STARTING_PRICE  = "$STARTING_PRICE";  // ex: "$99.00"
    const std::string CURRENT_BID     = "$CURRENT_BID";     // ex: "$120.00"
    const std::string END_ISO         = "";                 // ex: "2025-05-01T17:30:00Z" (optional)

    std::cout << R"(
    <style>
      /* Page-local styles for the hero band and centered button */
      .bid-hero {
        background: linear-gradient(135deg, #111827, #1f2937 50%, #111827);
        color: #fff;
        border-radius: 12px;
        padding: 16px 18px;
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 12px;
      }
      .bid-hero h2 {
        margin: 0;
        font-size: 22px;
        letter-spacing: .3px;
      }
      /* Condition pill REMOVED on request — intentionally unused */
      .cond-pill { display: none; }

      .bid-grid {
        display: grid;
        grid-template-columns: 1fr 1fr;
        gap: 18px;
      }
      .price-box {
        background: #fff;
        border: 1px solid var(--border);
        border-radius: 12px;
        padding: 16px;
      }
      .stat-label {
        color: #64748b;
        font-size: 14px;
        margin-bottom: 6px;
      }
      .stat-value {
        font-weight: 800;
        font-size: 18px;
        letter-spacing: .2px;
        color: #0f172a;
      }
      .bid-form {
        margin-top: 12px;
        border-top: 1px solid var(--border);
        padding-top: 12px;
      }
      .bid-row {
        display: grid;
        grid-template-columns: 1fr auto; /* input, then button */
        gap: 10px;
        align-items: center;
      }
      .bid-actions {
        display: flex;
        justify-content: center; /* center the Place Bid button */
        margin-top: 10px;
      }
      .bid-note, .bid-tip { color: var(--muted); font-size: 13px; }
      .banner { margin: 10px 0; }
    </style>

    <!-- (Optional) Inline banner areas (toggle display via backend) -->
    <div id="bid-error" class="banner error" style="display:none;">Please enter a bid amount.</div>
    <div id="bid-ok" class="banner success" style="display:none;">Your bid was placed.</div>

    <section class="card">
)";

    // Hero band: item name only (condition pill removed)
    std::cout
        << "      <div class='bid-hero'>\n"
        << "        <div>\n"
        << "          <h2>" << htmlEscape(ITEM_NAME) << "</h2>\n"
        << "          <div class='muted' style='margin-top:2px;'>Item detail page</div>\n"
        << "        </div>\n"
        << "        <!-- condition pill intentionally removed -->\n"
        << "      </div>\n";

    // Description
    std::cout << R"(
      <h3 style="margin:18px 0 8px;">Description</h3>
)";
    std::cout << "      <div class='muted' style='white-space:pre-wrap;'>"
              << htmlEscape(ITEM_DESC) << "</div>\n";

    // Prices + form
    std::cout << R"(
      <div class="price-box" style="margin-top:16px;">
        <div class="bid-grid">
          <div>
            <div class="stat-label">Starting price</div>
            <div class="stat-value">)";

    std::cout << htmlEscape(STARTING_PRICE) << R"(</div>
            <div class="bid-note" id="endLine">Ends: TBD</div>
          </div>

          <div>
            <div class="stat-label">Current bid</div>
            <div class="stat-value">)";

    std::cout << htmlEscape(CURRENT_BID) << R"(</div>
          </div>
        </div>

        <div class="bid-form">
          <div class="stat-label">Your bid</div>
          <div class="bid-row">
            <input id="bidAmount" name="amount" type="number" inputmode="decimal" step="0.01" min="0" placeholder="0.00">
          </div>

          <div class="bid-actions">
            <button class="btn primary" id="placeBidBtn" type="button">Place<br>Bid</button>
          </div>

          <div class="bid-note" style="margin-top:10px;">
            Your bid must be greater than the current bid.
          </div>
          <div class="bid-tip" style="margin-top:4px;">
            Tip: Typical increments are $1.00.
          </div>
        </div>
      </div>
    </section>
)";

    // Small helper JS: update "Ends:" if END_ISO is provided (no backend required)
    std::cout << R"(
    <script>
      (function () {
        var iso = )";

    // Emit the ISO string safely into JS
    std::cout << "\"" << htmlEscape(END_ISO) << "\"";

    std::cout << R"(;
        var endLine = document.getElementById('endLine');
        if (iso && endLine) {
          var d = new Date(iso);
          if (!isNaN(d.getTime())) {
            var pad = n => (n<10?'0':'') + n;
            var txt = d.getFullYear() + '-' + pad(d.getMonth()+1) + '-' + pad(d.getDate())
                    + ' ' + pad(d.getHours()) + ':' + pad(d.getMinutes());
            endLine.textContent = 'Ends: ' + txt + ' (local time)';
          }
        }

        // Demo-only click (no backend): show error if empty
        var btn = document.getElementById('placeBidBtn');
        var amt = document.getElementById('bidAmount');
        var ok  = document.getElementById('bid-ok');
        var er  = document.getElementById('bid-error');
        if (btn && amt) {
          btn.addEventListener('click', function(){
            if (!amt.value) {
              if (er) er.style.display = 'block';
              if (ok) ok.style.display = 'none';
            } else {
              if (ok) ok.style.display = 'block';
              if (er) er.style.display = 'none';
            }
          });
        }
      })();
    </script>
)";

    printTail();
}

// -------------------------------------------------------------
// POST — Placeholder (so linking succeeds). Backend will:
//  - read POST fields
//  - validate amount > current bid
//  - write bid
//  - redirect/render with status
// -------------------------------------------------------------
void BidPage::handlePost() {
    // For now, just re-render the template.
    handleGet();
}
