// src/pages/BrowsePage.cpp
#include "pages/BrowsePage.hpp"
#include "utils/utils.hpp"
#include <iostream>

BrowsePage::BrowsePage(Database& db, Session& session)
    : Page(db, session) {}

void BrowsePage::handleGet() {
    sendHTMLHeader();
    printHead("Browse Auctions · Team Elevate Auctions");

    std::cout << R"(
<section class="card" aria-labelledby="browse-heading">
  <h2 id="browse-heading" style="margin-top:0">Browse Auctions</h2>

  <!-- Top controls (visual only; backend can wire up later) -->
  <div style="display:flex; gap:12px; flex-wrap:wrap; margin-bottom:12px;">
    <input type="search" placeholder="Search items..." aria-label="Search items"
           style="flex:1; min-width:220px; padding:10px 12px; border:1px solid var(--border); border-radius:12px; font-size:14px; background:#fff;">
    <select aria-label="Sort"
            style="padding:10px 12px; border:1px solid var(--border); border-radius:12px; background:#fff; min-width:180px;">
      <option>Sort: Ending soon</option>
      <option>Sort: Newest</option>
      <option>Sort: Lowest price</option>
      <option>Sort: Highest price</option>
    </select>
  </div>

  <!-- Scrollable list area -->
  <div class="table-wrap" style="
        border:1px solid var(--border);
        border-radius:12px;
        overflow:hidden;
        background:#fff;
      ">
    <table aria-label="Auction items list" style="margin:0;">
      <colgroup>
        <col style="width:50%;"> <!-- Item -->
        <col style="width:20%;"> <!-- Seller -->
        <col style="width:15%;"> <!-- Current Bid -->
        <col style="width:15%;"> <!-- Time Left -->
      </colgroup>
      <thead>
        <tr>
          <th scope="col">Item</th>
          <th scope="col">Seller</th>
          <th scope="col">Current&nbsp;Bid</th>
          <th scope="col">Time&nbsp;Left</th>
        </tr>
      </thead>
    </table>

    <!-- Body in its own scroll region so header stays fixed -->
    <div id="rowsViewport" style="max-height:420px; overflow:auto; border-top:1px solid var(--border);">
      <table style="margin:0; border-spacing:0;">
        <colgroup>
          <col style="width:50%;"> <!-- Item -->
          <col style="width:20%;"> <!-- Seller -->
          <col style="width:15%;"> <!-- Current Bid -->
          <col style="width:15%;"> <!-- Time Left -->
        </colgroup>
        <tbody id="js-rows">
          <!-- hidden placeholder row so <col> has cells; removed once real rows appear -->
          <tr id="placeholderRow" style="visibility:collapse;">
            <td></td><td></td><td></td><td></td>
          </tr>

          <!-- Backend later: inject one <tr> per item here (no pagination; inject ALL rows).
               Columns (4):
               <td>ITEM_NAME</td>
               <td>SELLER_NAME</td>
               <td>$CURRENT_BID</td>
               <td>TIME_LEFT</td>
          -->
          <!-- Example row:
          <tr>
            <td><a href="/cgi-bin/item.cgi?id=123">Nintendo Switch OLED</a></td>
            <td><a href="/cgi-bin/user.cgi?u=jdoe">jdoe</a></td>
            <td>$215.00</td>
            <td>02:13:45</td>
          </tr>
          -->
        </tbody>
      </table>

      <!-- Empty state (shown only when there are no rows) -->
      <div id="emptyState" style="padding:14px; color:var(--muted); display:none;">
        No items to show yet.
      </div>
    </div>
  </div>

  <!-- No pagination controls; scrolling handles browsing all items. -->
</section>

<script>
(function () {
  // Manage empty state and remove placeholder row once real rows exist.
  var tbody = document.getElementById('js-rows');
  var emptyState = document.getElementById('emptyState');
  var placeholder = document.getElementById('placeholderRow');

  function updateEmptyState() {
    // real rows = any <tr> except the placeholder
    var rows = Array.prototype.filter.call(
      tbody.querySelectorAll('tr'),
      function (tr) { return tr !== placeholder; }
    );

    // remove placeholder once we have real rows
    if (rows.length > 0 && placeholder && placeholder.parentNode) {
      placeholder.parentNode.removeChild(placeholder);
      placeholder = null;
    }

    emptyState.style.display = rows.length > 0 ? 'none' : 'block';
  }

  // If backend injects rows after load, call window.refreshBrowse()
  window.refreshBrowse = updateEmptyState;

  // Initial check
  updateEmptyState();
})();
</script>
)";

    printTail();
}
