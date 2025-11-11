// pages/IndexPage.cpp
#include "pages/IndexPage.hpp"
#include "utils/utils.hpp"
#include <iostream>

IndexPage::IndexPage(Database& db, Session& session)
    : Page(db, session) {
}

void IndexPage::handleGet() {
    std::cout << "Content-type: text/html\n\n";

    std::string userEmail;
    bool isLoggedIn = session_.validate();

    if (isLoggedIn) {
        userEmail = session_.userEmail();
    }

    // ---------------------------------------------------------
    // HTML Output
    // ---------------------------------------------------------
    std::cout << R"(<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="refresh" content="305">
  <title>Team Elevate — Auctions</title>
  <style>
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

    header {
      background: linear-gradient(135deg, #111827, #1f2937 50%, #111827);
      color: #fff;
    }
    .nav { display: flex; align-items: center; justify-content: space-between; padding: 18px 0; gap: 16px; }
    .brand { display: flex; align-items: center; gap: 12px; }

    /* Logo: using PNG */
    .logo {
      width: 48px;                 /* was 36px */
      height: 48px;                /* was 36px */
      border-radius: 10px;
      background: url("../images/E8.png") center / contain no-repeat;
      background-color: transparent;
    }
    @media (max-width: 720px) {
      .logo { width: 36px; height: 36px; }
    }

    .brand h1 { margin: 0; font-size: 18px; letter-spacing: .3px; }

    /* Make brand text a link with no underline (even on hover) */
    .brand a.brand-link { color:#fff; text-decoration:none; }
    .brand a.brand-link:hover { text-decoration:none; }

    .links { display: flex; gap: 14px; align-items: center; flex-wrap: wrap; }
    .links a { color: #e5e7eb; text-decoration: none; font-weight: 600; padding: 6px 8px; border-radius: 8px; }
    .links a:hover { color: #fff; background: rgba(255,255,255,.08); }

    .hero { display: grid; grid-template-columns: 1.2fr .8fr; gap: 28px; align-items: center; padding: 28px 0 42px; }
    .hero h2 { margin: 0 0 12px; font-size: clamp(28px, 4vw, 36px); line-height: 1.15; }
    .hero p { margin: 0; color: #cbd5e1; }

    .cta { margin-top: 18px; display: flex; gap: 12px; flex-wrap: wrap; }
    .btn { display: inline-block; padding: 10px 14px; border-radius: 10px; text-decoration: none; font-weight: 700; }
    .btn.primary { background: var(--brand); color: #fff; }
    .btn.ghost { background: rgba(255,255,255,.1); color: #fff; border: 1px solid rgba(255,255,255,.25); }

    main { padding: 22px 0; }
    .card { background: var(--card); border-radius: var(--radius); box-shadow: var(--shadow); padding: 18px; border: 1px solid var(--border); }
    .card h3 { margin-top: 0; color: #374151; }

    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 16px; margin-top: 16px; }
    .feature h3 { margin: 0 0 6px; font-size: 18px; }
    .muted { color: var(--muted); }
    .user-info { background: #eef6ff; padding: 10px 12px; border-radius: 10px; margin: 14px 0; border: 1px solid #dbeafe; }
    footer { margin: 28px 0 22px; color: var(--muted); font-size: 14px; }
    @media (prefers-reduced-motion: reduce) { * { transition: none !important; } }
    @media (max-width: 900px) { .hero { grid-template-columns: 1fr; } }
  </style>
</head>
<body>
  <a class="skip-link" href="#main">Skip to content</a>
  <header>
    <div class="container">
      <div class="nav" role="navigation" aria-label="Primary">
        <div class="brand">
          <span class="logo" aria-hidden="true"></span>
          <h1><a class="brand-link" href="index.cgi">Team Elevate Auctions</a></h1>
        </div>
        <nav class="links">
)";

    if (isLoggedIn) {
        std::cout << R"(
          <a href="browse.cgi">Browse Auctions</a>
          <a href="bid.cgi">Bid</a>
          <a href="sell.cgi">Sell</a>
          <a href="transactions.cgi">My Transactions</a>
          <a href="logout.cgi">Logout</a>
)";
    }
    else {
        // NOTE:  when logged out
        std::cout << R"(
          <a href="browse.cgi">Browse Auctions</a>
          <a href="bid.cgi">Bid</a>
          <a href="sell.cgi">Sell</a>
          <a href="login.cgi">Login</a>
          <a href="register.cgi">Register</a>
)";
    }

    std::cout << R"(        </nav>
      </div>

      <section class="hero" aria-label="Welcome">
        <div>
          <h2>Bid. Win. Elevate.</h2>
          <p>Trusted listings, transparent bidding, and real-time results — all in one place.</p>
          <div class="cta">
)";

    if (!isLoggedIn) {
        std::cout << R"(
            <a class='btn primary' href='register.cgi'>Create an account</a>
            <a class='btn ghost' href='login.cgi'>Log in</a>
)";
    }
    else {
        std::cout << R"(
            <a class='btn primary' href='browse.cgi'>Browse Auctions</a>
            <a class='btn ghost' href='transactions.cgi'>View my Transactions</a>
)";
    }

    std::cout << R"(
          </div>
        </div>

        <aside class="card" aria-label="Today's Highlights">
          <h3>Today's Highlights</h3>
          <ul style="margin:8px 0 0; padding-left:18px; color:#374151;">
            <li>Live updates as bids change</li>
            <li>Verified sellers &amp; secure checkout</li>
            <li>Watchlist to track your favorites</li>
          </ul>
        </aside>
      </section>
    </div>
  </header>

  <main id="main">
    <div class="container">
)";

    if (isLoggedIn) {
        std::cout << "  <div class='user-info'>\n"
            << "    ✓ Logged in as: <strong>" << htmlEscape(userEmail) << "</strong>\n"
            << "  </div>\n";
    }

    std::cout << R"(
      <section class="card">
        <h2 style="margin-top:0">Why choose Team Elevate?</h2>
        <div class="grid">
          <div class="feature">
            <h3>Fair &amp; Transparent</h3>
            <p class="muted">Clear rules, visible bid history, and no hidden fees.</p>
          </div>
          <div class="feature">
            <h3>Fast &amp; Secure</h3>
            <p class="muted">Optimized checkout and encrypted sessions keep you safe.</p>
          </div>
          <div class="feature">
            <h3>Built for You</h3>
            <p class="muted">Personalized watchlists and alerts so you never miss a win.</p>
          </div>
        </div>
      </section>

      <footer>
        &copy; 2025 Team Elevate. All rights reserved.
      </footer>
    </div>
  </main>
</body>
</html>
)";
}
