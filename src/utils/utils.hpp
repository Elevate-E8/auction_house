#ifndef UTILS_HPP
#define UTILS_HPP

#include <map>
#include <string>
#include <mysql/mysql.h>

// =============================================================
// Utility Functions — Team Elevate Auctions
// Shared helpers for CGI input parsing, encoding, hashing, etc.
// =============================================================

// -------------------------------------------------------------
// Cookie & Input Parsing
// -------------------------------------------------------------

// Extract a named cookie value from the HTTP_COOKIE string.
// Example: getCookieValue("session_token=abc; theme=dark", "session_token") -> "abc"
std::string getCookieValue(const std::string& cookies, const std::string& name);

// Parse standard application/x-www-form-urlencoded POST data
// from stdin into a key/value map.
std::map<std::string, std::string> parsePostData();

// Decode URL-encoded form strings (replaces %xx and '+').
std::string urlDecode(const std::string& str);

// -------------------------------------------------------------
// Security / Validation
// -------------------------------------------------------------

// Compute SHA-256 hash of a password (hex-encoded).
std::string hashPassword(const std::string& password);

// Generate a 32-character random hexadecimal session token.
std::string generateSessionToken();

// Basic syntax check for email format.
bool isValidEmail(const std::string& email);

// -------------------------------------------------------------
// HTML Utilities
// -------------------------------------------------------------

// Escape special HTML characters (&, <, >, ", ').
std::string htmlEscape(const std::string& s);
#endif