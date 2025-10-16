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

/* -------------------- Helpers -------------------- */

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

string hashPassword(const string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)password.c_str(), password.length(), hash);
    stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    return ss.str();
}

bool isValidEmail(const string& email) {
    size_t atPos = email.find('@');
    size_t dotPos = email.find('.', atPos);
    return (atPos != string::npos && dotPos != string::npos &&
        atPos > 0 && dotPos > atPos + 1 && dotPos < email.length() - 1);
}

string generateSessionToken() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 15);
    stringstream ss;
    for (int i = 0; i < 32; i++) ss << hex << dis(gen);
    return ss.str();
}

/* -------------------- UI Shell -------------------- */

void printHead(const string& title) {
    cout << "<!doctype html>\n<html lang='en'>\n<head>\n"
        << "  <meta charset='utf-8'>\n"
        << "  <meta name='viewport' content='width=device-width, initial-scale=1'>\n"
        << "  <title>" << title << "</title>\n"
        << "  <style>\n"
        << "    :root{--bg:#0f172a;--bg2:#111827;--card:#ffffff;--muted:#6b7280;"
        " --text:#0f172a;--ink:#111827;--ink2:#374151;--soft:#f3f4f6;--accent:#5b3bf1;--accent-ink:#ffffff;}\n"
        << "    *{box-sizing:border-box} body{margin:0;font-family:Inter,system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;background:#f3f4f8;color:var(--ink)}\n"
        << "    .topbar{background:linear-gradient(180deg,var(--bg),var(--bg2));color:#fff;padding:18px 24px;display:flex;align-items:center;justify-content:space-between}\n"
        << "    .brand{display:flex;align-items:center;gap:12px;font-weight:800;font-size:20px}\n"
        << "    .dot{width:22px;height:22px;border-radius:8px;background:var(--accent)}\n"
        << "    nav a{color:#e5e7eb;text-decoration:none;margin-left:22px;font-weight:600}\n"
        << "    nav a:hover{color:#fff}\n"
        << "    .wrap{max-width:980px;margin:32px auto;padding:0 20px}\n"
        << "    .card{background:var(--card);border-radius:18px;box-shadow:0 10px 25px rgba(31,41,55,.08);padding:28px}\n"
        << "    h1{margin:0 0 10px;font-size:28px;color:var(--ink)}\n"
        << "    p.muted{color:var(--muted);margin:0 0 20px}\n"
        << "    form{display:flex;flex-direction:column;gap:14px;margin-top:10px}\n"
        << "    label{font-weight:700;color:var(--ink2);font-size:14px}\n"
        << "    input{width:100%;padding:12px 14px;border:1px solid #e5e7eb;border-radius:12px;font-size:15px;background:#fff}\n"
        << "    input:focus{outline:2px solid var(--accent);border-color:transparent}\n"
        << "    .btn{display:inline-block;background:var(--accent);color:var(--accent-ink);border:none;border-radius:12px;padding:12px 16px;font-weight:700;cursor:pointer}\n"
        << "    .btn:hover{filter:brightness(1.05)}\n"
        << "    .error{background:#fef2f2;border:1px solid #fecaca;color:#991b1b;padding:12px 14px;border-radius:12px;margin:8px 0}\n"
        << "    .success{background:#ecfdf5;border:1px solid #a7f3d0;color:#065f46;padding:12px 14px;border-radius:12px;margin:8px 0}\n"
        << "    .helper{color:var(--muted);font-size:13px}\n"
        << "    .footer{color:#6b7280;font-size:13px;margin:36px 0;text-align:center}\n"
        << "    .top-gap{margin-top:34px}\n"
        << "  </style>\n"
        << "</head>\n<body>\n"
        << "<header class='topbar'>\n"
        << "  <div class='brand'><div class='dot'></div><div>Team Elevate Auctions</div></div>\n"
        << "  <nav>\n"
        << "    <a href='/~elevate/cgi/index.cgi'>Home</a>\n"
        << "    <a href='/~elevate/cgi/login.cgi'>Login</a>\n"
        << "    <a href='/~elevate/cgi/register.cgi'>Register</a>\n"
        << "  </nav>\n"
        << "</header>\n"
        << "<main class='wrap'>\n";
}

void printTail() {
    cout << "  <div class='footer'>&copy; 2025 Team Elevate. All rights reserved.</div>\n"
        << "</main>\n</body>\n</html>\n";
}

// --------- Add header output here for ANY HTML ---------
void sendHTMLHeader() {
    cout << "Content-Type: text/html\n\n";
}

void renderForm(const string& emailPrefill = "", const string& errorMsg = "") {
    sendHTMLHeader();  // Ensure header is sent first
    printHead("Create your account");
    cout << "  <section class='card'>\n"
        << "    <h1>Create your account</h1>\n"
        << "    <p class='muted'>Join Team Elevate to start bidding and tracking your favorites.</p>\n";
    if (!errorMsg.empty()) {
        cout << "    <div class='error'>" << errorMsg << "</div>\n";
    }
    cout << "    <form method='post' action='/~elevate/cgi/register.cgi'>\n"
        << "      <div>\n"
        << "        <label for='email'>Email</label>\n"
        << "        <input id='email' name='email' type='email' maxlength='100' required value='" << emailPrefill << "'>\n"
        << "      </div>\n"
        << "      <div>\n"
        << "        <label for='password'>Password</label>\n"
        << "        <input id='password' name='password' type='password' minlength='8' required>\n"
        << "        <div class='helper'>Use at least 8 characters.</div>\n"
        << "      </div>\n"
        << "      <div>\n"
        << "        <label for='confirm'>Confirm Password</label>\n"
        << "        <input id='confirm' name='confirm' type='password' minlength='8' required>\n"
        << "      </div>\n"
        << "      <button class='btn' type='submit'>Create account</button>\n"
        << "    </form>\n"
        << "    <div class='top-gap helper'>Already have an account? <a href='/~elevate/cgi/login.cgi'>Log in</a>.</div>\n"
        << "  </section>\n";
    printTail();
}

void renderSuccess(const string& email, const string& sessionToken) {
    cout << "Content-Type: text/html\n";
    cout << "Set-Cookie: session_token=" << sessionToken << "; Path=/; HttpOnly; SameSite=Lax\n\n";

    printHead("Registration Successful");
    cout << "  <section class='card'>\n"
        << "    <h1>✓ Registration Successful</h1>\n"
        << "    <div class='success'>Welcome to Team Elevate, " << email << ".</div>\n"
        << "    <p class='muted'>You’ll be redirected to the home page momentarily.</p>\n"
        << "    <meta http-equiv='refresh' content='3;url=/~elevate/cgi/index.cgi'>\n"
        << "    <div class='top-gap'><a class='btn' href='/~elevate/cgi/index.cgi'>Go now</a></div>\n"
        << "  </section>\n";
    printTail();
}

/* -------------------- Main -------------------- */

int main() {
    const char* requestMethod = getenv("REQUEST_METHOD");
    if (!requestMethod || string(requestMethod) != "POST") {
        renderForm();
        return 0;
    }

    map<string, string> postData = parsePostData();
    string email = postData["email"];
    string password = postData["password"];
    string confirm = postData["confirm"];

    if (email.empty() || password.empty() || confirm.empty()) {
        renderForm(email, "All fields are required.");
        return 0;
    }
    if (!isValidEmail(email)) {
        renderForm(email, "Please enter a valid email address.");
        return 0;
    }
    if (password.length() < 8) {
        renderForm(email, "Password must be at least 8 characters.");
        return 0;
    }
    if (password != confirm) {
        renderForm(email, "Passwords do not match.");
        return 0;
    }

    MYSQL* conn = mysql_init(NULL);
    if (!conn || !mysql_real_connect(conn, "localhost", "cs370_section2_elevate", "etavele_004",
        "cs370_section2_elevate", 0, NULL, 0)) {
        renderForm(email, "Internal server error. Please try again later.");
        if (conn) mysql_close(conn);
        return 0;
    }

    // CHECK IF EMAIL EXISTS (prepared statement)
    MYSQL_STMT* checkStmt = mysql_stmt_init(conn);
    const char* checkSql = "SELECT user_id FROM users WHERE user_email=? LIMIT 1";
    if (!checkStmt || mysql_stmt_prepare(checkStmt, checkSql, strlen(checkSql)) != 0) {
        renderForm(email, "Internal server error. Please try again later.");
        mysql_close(conn);
        return 0;
    }

    MYSQL_BIND checkParam[1];
    memset(checkParam, 0, sizeof(checkParam));
    checkParam[0].buffer_type = MYSQL_TYPE_STRING;
    checkParam[0].buffer = (char*)email.c_str();
    checkParam[0].buffer_length = email.size();

    if (mysql_stmt_bind_param(checkStmt, checkParam) != 0 || mysql_stmt_execute(checkStmt) != 0) {
        renderForm(email, "Internal server error. Please try again later.");
        mysql_stmt_close(checkStmt);
        mysql_close(conn);
        return 0;
    }

    MYSQL_BIND checkResult;
    memset(&checkResult, 0, sizeof(checkResult));
    my_ulonglong userIdDummy = 0;
    checkResult.buffer_type = MYSQL_TYPE_LONGLONG;
    checkResult.buffer = &userIdDummy;
    checkResult.is_unsigned = 1;

    if (mysql_stmt_bind_result(checkStmt, &checkResult) != 0 || mysql_stmt_store_result(checkStmt) != 0) {
        renderForm(email, "Internal server error. Please try again later.");
        mysql_stmt_close(checkStmt);
        mysql_close(conn);
        return 0;
    }

    int fetchStatus = mysql_stmt_fetch(checkStmt);
    mysql_stmt_close(checkStmt);

    if (fetchStatus == 0) {
        renderForm(email, "This email is already registered. Try a different one or log in.");
        mysql_close(conn);
        return 0;
    }

    // INSERT USER
    string hashedPassword = hashPassword(password);
    MYSQL_STMT* insertStmt = mysql_stmt_init(conn);
    const char* insertSql = "INSERT INTO users (user_email, password_hash, joindate) VALUES (?, ?, NOW())";
    if (!insertStmt || mysql_stmt_prepare(insertStmt, insertSql, strlen(insertSql)) != 0) {
        renderForm(email, "Internal server error. Please try again later.");
        mysql_close(conn);
        return 0;
    }

    MYSQL_BIND insertParam[2];
    memset(insertParam, 0, sizeof(insertParam));
    insertParam[0].buffer_type = MYSQL_TYPE_STRING;
    insertParam[0].buffer = (char*)email.c_str();
    insertParam[0].buffer_length = email.size();

    insertParam[1].buffer_type = MYSQL_TYPE_STRING;
    insertParam[1].buffer = (char*)hashedPassword.c_str();
    insertParam[1].buffer_length = hashedPassword.size();

    if (mysql_stmt_bind_param(insertStmt, insertParam) != 0 || mysql_stmt_execute(insertStmt) != 0) {
        renderForm(email, "Internal server error. Please try again later.");
        mysql_stmt_close(insertStmt);
        mysql_close(conn);
        return 0;
    }

    unsigned long long newUserId = mysql_stmt_insert_id(insertStmt);
    mysql_stmt_close(insertStmt);

    // CREATE SESSION
    string sessionToken = generateSessionToken();
    char* remoteAddr = getenv("REMOTE_ADDR");
    string ipAddress = remoteAddr ? string(remoteAddr) : "unknown";

    MYSQL_STMT* sessionStmt = mysql_stmt_init(conn);
    const char* sessionSql = "INSERT INTO sessions (user_id, session_token, login_time, last_active, ip_address) "
        "VALUES (?, ?, NOW(), NOW(), ?)";
    if (!sessionStmt || mysql_stmt_prepare(sessionStmt, sessionSql, strlen(sessionSql)) != 0) {
        renderForm(email, "Internal server error. Please try again later.");
        mysql_close(conn);
        return 0;
    }

    MYSQL_BIND sessionParam[3];
    memset(sessionParam, 0, sizeof(sessionParam));

    sessionParam[0].buffer_type = MYSQL_TYPE_LONGLONG;
    sessionParam[0].buffer = &newUserId;
    sessionParam[0].is_unsigned = 1;

    sessionParam[1].buffer_type = MYSQL_TYPE_STRING;
    sessionParam[1].buffer = (char*)sessionToken.c_str();
    sessionParam[1].buffer_length = sessionToken.size();

    sessionParam[2].buffer_type = MYSQL_TYPE_STRING;
    sessionParam[2].buffer = (char*)ipAddress.c_str();
    sessionParam[2].buffer_length = ipAddress.size();

    if (mysql_stmt_bind_param(sessionStmt, sessionParam) != 0 || mysql_stmt_execute(sessionStmt) != 0) {
        renderForm(email, "Internal server error. Please try again later.");
        mysql_stmt_close(sessionStmt);
        mysql_close(conn);
        return 0;
    }

    mysql_stmt_close(sessionStmt);
    mysql_close(conn);

    renderSuccess(email, sessionToken);

    return 0;
}
