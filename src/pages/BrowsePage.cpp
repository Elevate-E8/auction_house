// src/pages/BrowsePage.cpp
#include "pages/BrowsePage.hpp"
#include "utils/utils.hpp"
#include <iostream>

BrowsePage::BrowsePage(Database& db, Session& session)
    : Page(db, session) {}

void BrowsePage::handleGet() {
    sendHTMLHeader();
    printHead("Browse Auctions · Team Elevate Auctions");

    // (Optional) gate: if you ever want to force login to view this page,
    // check session_.validate() here and show a message instead.

    std::cout << R"(
<section class="card" aria-labelledby="browse-heading">
  <h2 id="browse-heading" style="margin-top:0">Browse Auctions</h2>

  <!-- Top controls (purely visual — backend may wire them up later) -->
  <div style="display:flex; gap:12px; flex-wrap:wrap; margin-bottom:12px;">
    <input type="search" placeholder="Search items..." aria-label="Search items"
           style="flex:1; min-width:220px; padding:10px 12px; border:1px solid var(--border); border-radius:12px; font-size:14px; background:#fff;" />
    <select aria-label="Filter by condition"
            style="padding:10px 12px; border:1px solid var(--border); border-radius:12px; background:#fff; min-width:180px;">
      <option value="">All conditions</option>
      <option>New</option>
      <option>Like New</option>
      <option>Good</option>
      <option>Fair</option>
      <option>Poor</option>
    </select>
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
    <table aria-describedby="browse-note" style="margin:0;">
      <colgroup>
        <col style="width:48%;">
        <col style="width:18%;">
        <col style="width:18%;">
        <col style="width:16%;">
      </colgroup>
      <thead>
        <tr>
          <th scope="col">Item</th>
          <th scope="col">Condition</th>
          <th scope="col">Current Bid</th>
          <th scope="col">Time Left</th>
        </tr>
      </thead>
    </table>

    <!-- Body in its own scroll region so header stays fixed -->
    <div id="rowsViewport" style="max-height:420px; overflow:auto; border-top:1px solid var(--border);">
      <table style="margin:0; border-spacing:0;">
        <colgroup>
          <col style="width:48%;">
          <col style="width:18%;">
          <col style="width:18%;">
          <col style="width:16%;">
        </colgroup>
        <tbody id="js-rows">
          <!-- Back-end DEVs: inject one <tr> per item here.
               Expected columns:
               <td>ITEM_NAME</td>
               <td>CONDITION (New / Like New / Good / Fair / Poor)</td>
               <td>$CURRENT_BID</td>
               <td>TIME_LEFT (e.g., 2d 3h or 05:18:12)</td>
          -->
        </tbody>
      </table>

      <!-- Empty state (shown only when there are no rows) -->
      <div id="emptyState" style="padding:14px; color:var(--muted); display:none;">
        No items to show yet.
      </div>
    </div>
  </div>

  <!-- Pagination (client-side only, no page reload) -->
  <div style="display:flex; align-items:center; justify-content:center; gap:16px; margin-top:12px;">
    <button id="pgPrev" class="btn" type="button"
            style="border:1px solid var(--border); background:#fff; padding:8px 14px;">Prev</button>
    <div id="pgLabel" class="muted">Page 1</div>
    <button id="pgNext" class="btn" type="button"
            style="border:1px solid var(--border); background:#fff; padding:8px 14px;">Next</button>
  </div>

  <p id="browse-note" class="helper" style="margin-top:12px;">
    This is a visual template. Backend will inject rows into the table body (<code>#js-rows</code>)
    and may wire search/filter/sort server-side later.
  </p>
</section>

<script>
(function () {
  // ----- Simple in-place pagination over whatever rows exist in #js-rows -----
  var tbody = document.getElementById('js-rows');
  var emptyState = document.getElementById('emptyState');
  var pgPrev = document.getElementById('pgPrev');
  var pgNext = document.getElementById('pgNext');
  var pgLabel = document.getElementById('pgLabel');

  var pageSize = 10;       // show up to 10 rows per page
  var current = 1;

  function allRows() {
    return Array.prototype.slice.call(tbody.querySelectorAll('tr'));
  }

  function render() {
    var rows = allRows();
    var total = rows.length;
    var pages = Math.max(1, Math.ceil(total / pageSize));

    // Clamp current
    if (current > pages) current = pages;
    if (current < 1) current = 1;

    // Show/hide empty state
    emptyState.style.display = total === 0 ? 'block' : 'none';

    // Hide all rows then show the slice for the current page
    rows.forEach(function (tr) { tr.style.display = 'none'; });
    var start = (current - 1) * pageSize;
    var end = start + pageSize;
    rows.slice(start, end).forEach(function (tr) { tr.style.display = ''; });

    // Controls
    pgPrev.disabled = (current === 1 || total === 0);
    pgNext.disabled = (current === pages || total === 0);
    pgLabel.textContent = 'Page ' + current;
  }

  pgPrev.addEventListener('click', function(){ current--; render(); });
  pgNext.addEventListener('click', function(){ current++; render(); });

  // If your backend injects rows after load, it can call window.refreshBrowse()
  // to recompute pagination without reloading the page.
  window.refreshBrowse = render;

  // Initial draw
  render();
})();
</script>
)";

    printTail();
}
