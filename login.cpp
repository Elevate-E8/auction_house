#include <iostream>
#include <string>
#include <map>
#include <mysql/mysql.h>
#include <cstdlib>
#include <sstream>
#include <openssl/sha.h>
#include <iomanip>
#include <random>
#include <cstring>

using namespace std;

// ----------------- URL Decode Helper -----------------
string urlDecode(const string& str) {
    string result;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '+') result += ' ';
        else if (str[i] == '%' && i + 2 < str.length()) {
            unsigned int value;
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &value);
            result += static_cast<char>(value);
            i += 2;
        }
        else result += str[i];
    }
    return result;
}

// ----------------- Parse POST Data -----------------
map<string, string> parsePostData() {
    map<string, string> data;
    char* contentLength = getenv("CONTENT_LENGTH");
    if (contentLength) {
        int length = atoi(contentLength);
        if (length > 0) {
            string postData;
            postData.resize(length);
            cin.read(&postData[0], length);

            stringstream ss(postData);
            string pair;
            while (getline(ss, pair, '&')) {
                size_t pos = pair.find('=');
                if (pos != string::npos) {
                    string key = pair.substr(0, pos);
                    string value = pair.substr(pos + 1);
                    data[key] = urlDecode(value);
                }
            }
        }
    }
    return data;
}

// ----------------- SHA-256 Password Hash -----------------
string hashPassword(const string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)password.c_str(), password.length(), hash);

    stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    return ss.str();
}

// ----------------- Session Token Generator -----------------
string generateSessionToken() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 15);
    stringstream ss;
    for (int i = 0; i < 32; i++) ss << hex << dis(gen);
    return ss.str();
}

// ----------------- Main -----------------
int main() {
    char* requestMethod = getenv("REQUEST_METHOD");

    // ----------------- GET: Show login form -----------------
    if (!requestMethod || string(requestMethod) != "POST") {
        cout << "Content-Type: text/html\n\n";
        cout
            << "<!DOCTYPE html>\n"
            "<html lang='en'>\n"
            "<head>\n"
            "  <meta charset='utf-8'>\n"
            "  <meta name='viewport' content='width=device-width, initial-scale=1'>\n"
            "  <title>Login · Team Elevate</title>\n"
            "  <style>\n"
            "    :root{--ink:#0f172a;--muted:#475569;--bg:#f6f7fb;--card:#ffffff;--brand:#4f46e5}\n"
            "    *{box-sizing:border-box}\n"
            "    html,body{margin:0;padding:0}\n"
            "    body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;color:var(--ink);background:var(--bg);line-height:1.5}\n"
            "    header{background:linear-gradient(135deg,#111827,#1f2937 50%,#111827);color:#fff}\n"
            "    .wrap{max-width:1024px;margin:0 auto;padding:0 20px}\n"
            "    .nav{display:flex;align-items:center;justify-content:space-between;padding:18px 0}\n"
            "    .brand{display:flex;gap:10px;align-items:center}\n"
            "    .logo{width:36px;height:36px;border-radius:10px;background:linear-gradient(135deg,var(--brand),#7c3aed)}\n"
            "    .brand h1{font-size:18px;margin:0;font-weight:700;letter-spacing:.3px}\n"
            "    .links a{color:#e5e7eb;text-decoration:none;margin-left:14px;font-weight:600}\n"
            "    .links a:hover{color:#fff;text-decoration:underline}\n"
            "    main{padding:28px 0}\n"
            "    .center{display:grid;place-items:center}\n"
            "    .card{background:var(--card);border-radius:16px;box-shadow:0 8px 22px rgba(2,6,23,.10);padding:22px;width:100%;max-width:440px}\n"
            "    .card h2{margin:0 0 4px;font-size:24px}\n"
            "    .sub{color:var(--muted);margin:0 0 16px;font-size:14px}\n"
            "    label{display:block;font-weight:600;margin:12px 0 6px}\n"
            "    input{width:100%;padding:10px 12px;border:1px solid #e5e7eb;border-radius:10px;background:#fff}\n"
            "    input:focus{outline:none;border-color:var(--brand);box-shadow:0 0 0 3px rgba(79,70,229,.15)}\n"
            "    .actions{margin-top:16px}\n"
            "    .btn{background:var(--brand);color:#fff;border:none;border-radius:14px;padding:14px 16px;font-weight:800;cursor:pointer;width:100%;display:block;text-align:center}\n"
            "    .btn:hover{filter:brightness(1.05)}\n"
            "    .muted{color:var(--muted)}\n"
            "    .helper{margin-top:12px;font-size:14px}\n"
            "    a{color:var(--brand);text-decoration:none}\n"
            "    a:hover{text-decoration:underline}\n"
            "  </style>\n"
            "</head>\n"
            "<body>\n"
            "  <header>\n"
            "    <div class='wrap'>\n"
            "      <div class='nav'>\n"
            "        <div class='brand'>\n"
            "          <span class='logo' aria-hidden='true'></span>\n"
            "          <h1>Team Elevate Auctions</h1>\n"
            "        </div>\n"
            "        <nav class='links'>\n"
            "          <a href='/~elevate/cgi/index.cgi'>Home</a>\n"
            "          <a href='/~elevate/cgi/register.cgi'>Register</a>\n"
            "        </nav>\n"
            "      </div>\n"
            "    </div>\n"
            "  </header>\n"
            "  <main class='center'>\n"
            "    <div class='card' role='form' aria-labelledby='login-title'>\n"
            "      <h2 id='login-title'>Welcome back</h2>\n"
            "      <p class='sub'>Log in to continue bidding.</p>\n"
            "      <form method='post' action='/~elevate/cgi/login.cgi' novalidate>\n"
            "        <label for='email'>Email</label>\n"
            "        <input id='email' name='email' type='email' maxlength='100' required autocomplete='email'>\n"
            "        <label for='password'>Password</label>\n"
            "        <input id='password' name='password' type='password' minlength='8' required autocomplete='current-password'>\n"
            "        <div class='actions'>\n"
            "          <button class='btn' type='submit'>Log In</button>\n"
            "        </div>\n"
            "      </form>\n"
            "      <p class='helper muted'>Don't have an account? <a href='/~elevate/cgi/register.cgi'>Create one</a>.</p>\n"
            "    </div>\n"
            "  </main>\n"
            "</body>\n"
            "</html>\n";
        return 0;
    }

    // ----------------- POST: authenticate -----------------
    map<string, string> postData = parsePostData();
    string email = postData["email"];
    string password = postData["password"];

    if (email.empty() || password.empty()) {
        // Display error
        cout << "Content-Type: text/html\n\n";
        cout << "<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'><title>Error</title>"
            "<style>:root{--ink:#0f172a;--muted:#475569;--bg:#f6f7fb;--card:#ffffff;--brand:#4f46e5}body{font-family:system-ui,Segoe UI,Roboto,Arial,sans-serif;max-width:560px;margin:48px auto;padding:0 16px}.card{background:#fff;border-radius:14px;box-shadow:0 8px 22px rgba(2,6,23,.10);padding:18px}.btn{display:inline-block;background:#4f46e5;color:#fff;text-decoration:none;font-weight:800;padding:12px 16px;border-radius:14px}</style>"
            "</head><body><div class='card'><h1>Missing fields</h1><p>Email and password are required.</p>"
            "<p><a class='btn' href='/~elevate/cgi/login.cgi'>Go back</a></p></div></body></html>";
        return 1;
    }

    // ----------------- Connect to MariaDB -----------------
    MYSQL* conn = mysql_init(NULL);
    if (!conn || !mysql_real_connect(conn, "localhost", "cs370_section2_elevate", "etavele_004",
        "cs370_section2_elevate", 0, NULL, 0)) {
        cout << "Content-Type: text/html\n\n";
        cout << "<!DOCTYPE html><html><body><h1>Database error</h1></body></html>";
        if (conn) mysql_close(conn);
        return 1;
    }

    // ----------------- Hash password -----------------
    string hashedPassword = hashPassword(password);

    // ----------------- Prepared statement for login -----------------
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    const char* sql = "SELECT user_id FROM users WHERE user_email=? AND password_hash=? LIMIT 1";
    if (!stmt || mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
        cout << "Content-Type: text/html\n\n";
        cout << "<!DOCTYPE html><html><body><h1>Internal server error</h1></body></html>";
        mysql_close(conn);
        return 1;
    }

    MYSQL_BIND param[2];
    memset(param, 0, sizeof(param));

    param[0].buffer_type = MYSQL_TYPE_STRING;
    param[0].buffer = (char*)email.c_str();
    param[0].buffer_length = email.size();

    param[1].buffer_type = MYSQL_TYPE_STRING;
    param[1].buffer = (char*)hashedPassword.c_str();
    param[1].buffer_length = hashedPassword.size();

    if (mysql_stmt_bind_param(stmt, param) != 0 || mysql_stmt_execute(stmt) != 0) {
        cout << "Content-Type: text/html\n\n";
        cout << "<!DOCTYPE html><html><body><h1>Internal server error</h1></body></html>";
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return 1;
    }

    MYSQL_BIND result;
    memset(&result, 0, sizeof(result));
    my_ulonglong userId = 0;
    result.buffer_type = MYSQL_TYPE_LONGLONG;
    result.buffer = &userId;
    result.is_unsigned = 1;

    if (mysql_stmt_bind_result(stmt, &result) != 0 || mysql_stmt_store_result(stmt) != 0) {
        cout << "Content-Type: text/html\n\n";
        cout << "<!DOCTYPE html><html><body><h1>Internal server error</h1></body></html>";
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return 1;
    }

    int fetchStatus = mysql_stmt_fetch(stmt);
    mysql_stmt_close(stmt);

    if (fetchStatus == 0) {
        // ----------------- LOGIN SUCCESS -----------------
        string sessionToken = generateSessionToken();
        char* remoteAddr = getenv("REMOTE_ADDR");
        string ipAddress = remoteAddr ? string(remoteAddr) : "unknown";

        MYSQL_STMT* insertStmt = mysql_stmt_init(conn);
        const char* insertSql = "INSERT INTO sessions (user_id, session_token, login_time, last_active, ip_address) VALUES (?, ?, NOW(), NOW(), ?)";
        if (insertStmt) {
            mysql_stmt_prepare(insertStmt, insertSql, strlen(insertSql));
            MYSQL_BIND insertParam[3];
            memset(insertParam, 0, sizeof(insertParam));

            insertParam[0].buffer_type = MYSQL_TYPE_LONGLONG;
            insertParam[0].buffer = &userId;
            insertParam[0].is_unsigned = 1;

            insertParam[1].buffer_type = MYSQL_TYPE_STRING;
            insertParam[1].buffer = (char*)sessionToken.c_str();
            insertParam[1].buffer_length = sessionToken.size();

            insertParam[2].buffer_type = MYSQL_TYPE_STRING;
            insertParam[2].buffer = (char*)ipAddress.c_str();
            insertParam[2].buffer_length = ipAddress.size();

            mysql_stmt_bind_param(insertStmt, insertParam);
            mysql_stmt_execute(insertStmt);
            mysql_stmt_close(insertStmt);
        }

        mysql_close(conn);

        // ----------------- success page -----------------
        cout << "Content-Type: text/html\n";
        cout << "Set-Cookie: session_token=" << sessionToken << "; Path=/; HttpOnly\n\n";
        cout
            << "<!DOCTYPE html>\n"
            "<html lang='en'>\n"
            "<head>\n"
            "  <meta charset='utf-8'>\n"
            "  <meta name='viewport' content='width=device-width, initial-scale=1'>\n"
            "  <title>Welcome · Team Elevate</title>\n"
            "  <meta http-equiv='refresh' content='2; url=/~elevate/cgi/index.cgi'>\n"
            "  <style>\n"
            "    :root{--ink:#0f172a;--muted:#475569;--bg:#f6f7fb;--card:#ffffff;--brand:#4f46e5}\n"
            "    *{box-sizing:border-box}\n"
            "    html,body{margin:0;padding:0}\n"
            "    body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;color:var(--ink);background:var(--bg);line-height:1.5}\n"
            "    main{display:grid;place-items:center;padding:22px 0}\n"
            "    .card{background:var(--card);border-radius:16px;box-shadow:0 8px 22px rgba(2,6,23,.10);padding:20px;width:100%;max-width:560px;text-align:center}\n"
            "    .title{margin:6px 0 4px;font-size:24px}\n"
            "    .muted{color:var(--muted)}\n"
            "    .btn{display:block;margin-top:14px;padding:14px 16px;border-radius:14px;background:var(--brand);color:#fff;font-weight:800;text-decoration:none}\n"
            "    .btn:hover{filter:brightness(1.05)}\n"
            "  </style>\n"
            "</head>\n"
            "<body>\n"
            "  <main>\n"
            "    <div class='card'>\n"
            "      <h2 class='title'>Login successful</h2>\n"
            "      <p class='muted'>Signed in as <strong>" << email << "</strong>. Redirecting to the homepage…</p>\n"
            "      <a class='btn' href='/~elevate/cgi/index.cgi'>Go to Home</a>\n"
            "    </div>\n"
            "  </main>\n"
            "</body>\n"
            "</html>\n";
    }
    else {
        // ----------------- LOGIN FAILURE -----------------
        cout << "Content-Type: text/html\n\n";
        cout
            << "<!DOCTYPE html>\n"
            "<html lang='en'>\n"
            "<head>\n"
            "  <meta charset='utf-8'>\n"
            "  <meta name='viewport' content='width=device-width, initial-scale=1'>\n"
            "  <title>Login Failed · Team Elevate</title>\n"
            "  <style>\n"
            "    :root{--ink:#0f172a;--muted:#475569;--bg:#f6f7fb;--card:#ffffff;--brand:#4f46e5}\n"
            "    *{box-sizing:border-box}\n"
            "    html,body{margin:0;padding:0}\n"
            "    body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;color:var(--ink);background:var(--bg);line-height:1.5}\n"
            "    main{display:grid;place-items:center;padding:22px 0}\n"
            "    .card{background:var(--card);border-radius:16px;box-shadow:0 8px 22px rgba(2,6,23,.10);padding:22px;width:100%;max-width:560px;text-align:center}\n"
            "    .title{margin:6px 0 8px;font-size:24px}\n"
            "    .error{color:#dc2626;margin:8px 0 16px}\n"
            "    .row{display:flex;gap:12px;flex-wrap:wrap;justify-content:center}\n"
            "    .btn{flex:1 1 240px;display:block;padding:14px 16px;border-radius:14px;background:var(--brand);color:#fff;font-weight:800;text-decoration:none;text-align:center}\n"
            "    .btn:hover{filter:brightness(1.05)}\n"
            "  </style>\n"
            "</head>\n"
            "<body>\n"
            "  <main>\n"
            "    <div class='card'>\n"
            "      <h2 class='title'>Login Failed</h2>\n"
            "      <p class='error'>Invalid email or password.</p>\n"
            "      <div class='row'>\n"
            "        <a class='btn' href='/~elevate/cgi/login.cgi'>Try again</a>\n"
            "        <a class='btn' href='/~elevate/cgi/register.cgi'>Create an account</a>\n"
            "      </div>\n"
            "    </div>\n"
            "  </main>\n"
            "</body>\n"
            "</html>\n";

        mysql_close(conn);
    }

    return 0;
}
