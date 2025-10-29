#ifndef UTILS_HPP
#define UTILS_HPP

#include <map>
#include <string>
#include <mysql/mysql.h>

// Helper utilities for Elevate Auction project.
// Added to improve readability and maintainability.

// Extracts a named cookie value from the HTTP_COOKIE string.
// Example: getCookieValue("session_token=abc; theme=dark", "session_token") -> "abc"
std::string getCookieValue(const std::string& cookies, const std::string& name);

// Creates and returns a new DB connection.
// Returns nullptr if connection fails.
MYSQL* createDBConnection();

// Frees a DB connection safely (nullptr-safe).
void closeDBConnection(MYSQL* conn);

// URL-decodes POST form-encoded strings (replaces %xx and +).
std::string urlDecode(const std::string& str);

// Parses standard application/x-www-form-urlencoded POST data.
std::map<std::string, std::string> parsePostData();

// Returns a SHA-256 hash (hex-encoded) of a password string.
std::string hashPassword(const std::string& password);

// Generates a 32-character random hexadecimal session token.
std::string generateSessionToken();

// Validates email format (basic syntax check).
bool isValidEmail(const std::string& email);

// -------------------- HTML Rendering Helpers --------------------

// Sends standard HTTP header for HTML pages.
void sendHTMLHeader();

// Prints shared page header with consistent layout and styles.
void printHead(const std::string& title, const std::string& layout = "content");

// Prints shared footer used across all pages.
void printTail(const std::string& layout = "content");

// ---- Session Utilities ----
bool isUserLoggedIn(std::string& userEmail);

#endif