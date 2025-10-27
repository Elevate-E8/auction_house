#include <iostream>
using namespace std;

int main() {
  cout << "Content-Type: text/html\n\n";

  cout << R"(<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="refresh" content="305">
  <title>Team Elevate — Auctions</title>
  <style>
    /* ===== Theme & Resets ================================================= */
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

    /* ===== Header / Nav (kept from home page) ============================= */
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

    /* ===== Page structure/cards =========================================== */
    main { padding: 22px 0; }
    .card { background: var(--card); border-radius: var(--radius); box-shadow: var(--shadow); padding: 18px; border: 1px solid var(--border); }
    .muted { color: var(--muted); }

    /* Layout: full-width Current Bids on top; three cards under it */
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

    /* ===== Tables ========================================================== */
    table{ width: 100%; border-collapse: separate; border-spacing: 0; table-layout: fixed; }
    thead th{ background: #f9fafb; border-bottom: 1px solid var(--border); padding: 10px 12px; text-align: left; font-weight: 700; }
    tbody td{ padding: 12px; border-top: 1px solid var(--border); vertical-align: middle; }
    /* subtle vertical dividers improve scan */
    .cbids th+th, .cbids td+td{ border-left: 1px solid var(--border); }

    /* Column widths for Current Bids */
    .cbids col:nth-child(1){ width: 44%; }
    .cbids col:nth-child(2){ width: 18%; }
    .cbids col:nth-child(3){ width: 18%; }
    .cbids col:nth-child(4){ width: 20%; }

    /* Center the numeric columns */
    .cbids th:nth-child(2), .cbids th:nth-child(3),
    .cbids td:nth-child(2), .cbids td:nth-child(3){ text-align: center; }

    /* ===== Action cell form: stays inside cell ============================ */
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

    /* ===== Footer & helpers =============================================== */
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
          <a href="my_bids.cgi" aria-current="page">My Bids</a>
          <a href="logout.cgi">Logout</a>
        </nav>
      </div>
    </div>
  </header>

  <main id="main">
    <div class="container">
      <!-- Intro card (matches home page style) -->
      <section class="card">
        <h2 style="margin:0 0 8px">My Transactions</h2>
        <p class="muted">Your selling activity, purchases, current bids, and items you didn’t win.</p>
      </section>

      <section class="layout" aria-label="Transactions">
        <!-- Top: Current Bids (full width) -->
        <article class="card bids">
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
              <!-- Example rows (replace with CGI data) -->
              <tr>
                <td>— (Outbid)</td>
                <td>$—.—</td>
                <td>$—.—</td>
                <td>
                  <form class="inline-form" action="bid.cgi" method="post">
                    <input type="hidden" name="item_id" value="">
                    <label class="visually-hidden" for="bid1">Increase max bid</label>
                    <input id="bid1" name="bid_amount" type="number" step="0.01" min="0" placeholder="Enter new max" required>
                    <button class="btn primary" type="submit">Increase</button>
                  </form>
                  <p class="muted" style="margin:.6rem 0 0" role="status">
                    If you are not highest bidder, an outbid notice will appear here.
                  </p>
                </td>
              </tr>
              <tr>
                <td>— (Leading)</td>
                <td>$—.—</td>
                <td>$—.—</td>
                <td>
                  <form class="inline-form" action="bid.cgi" method="post">
                    <input type="hidden" name="item_id" value="">
                    <label class="visually-hidden" for="bid2">Increase max bid</label>
                    <input id="bid2" name="bid_amount" type="number" step="0.01" min="0" placeholder="Enter new max">
                    <button class="btn primary" type="submit">Increase</button>
                  </form>
                </td>
              </tr>
            </tbody>
          </table>
        </article>

        <!-- Bottom row: three cards -->
        <article class="card sell">
          <h3 style="margin-top:0">Selling</h3>
          <table aria-label="Items you are selling">
            <thead><tr><th>Item</th><th>Status</th><th>Ends</th></tr></thead>
            <tbody><tr><td>—</td><td>—</td><td>—</td></tr></tbody>
          </table>
        </article>

        <article class="card purch">
          <h3 style="margin-top:0">Purchases</h3>
          <table aria-label="Items you purchased">
            <thead><tr><th>Item</th><th>Winning Bid</th><th>Closed</th></tr></thead>
            <tbody><tr><td>—</td><td>—</td><td>—</td></tr></tbody>
          </table>
        </article>

        <article class="card lost">
          <h3 style="margin-top:0">Didn’t Win</h3>
          <table aria-label="Auctions you didn’t win">
            <thead><tr><th>Item</th><th>Winning Bid</th><th>Closed</th></tr></thead>
            <tbody><tr><td>—</td><td>$—.—</td><td>—</td></tr></tbody>
          </table>
        </article>
      </section>

      <footer>&copy; 2025 Team Elevate. All rights reserved.</footer>
    </div>
  </main>
</body>
</html>
)";

  return 0;
}
