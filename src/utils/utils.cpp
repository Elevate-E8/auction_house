#include "utils.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <random>
#include <openssl/sha.h>
#include <cstring>
#include <cstdlib>

// ----------------- Extract Cookie -----------------
std::string getCookieValue(const std::string& cookies, const std::string& name) {
    size_t start = cookies.find(name + "=");
    if (start == std::string::npos)
        return "";
    start += name.length() + 1;
    size_t end = cookies.find(";", start);
    if (end == std::string::npos)
        end = cookies.length();
    return cookies.substr(start, end - start);
}

// ----------------- HTML Escape Helper -----------------
std::string htmlEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '&':  out += "&amp;";  break;
        case '<':  out += "&lt;";   break;
        case '>':  out += "&gt;";   break;
        case '"':  out += "&quot;"; break;
        case '\'': out += "&#x27;"; break;
        default:   out += c;        break;
        }
    }
    return out;
}

// ----------------- URL Decode Helper -----------------
std::string urlDecode(const std::string& str) {
    std::string result;
    for (std::size_t i = 0; i < str.length(); i++) {
        if (str[i] == '+') result += ' ';
        else if (str[i] == '%' && i + 2 < str.length()) {
            unsigned int value;
            std::sscanf(str.substr(i + 1, 2).c_str(), "%x", &value);
            result += static_cast<char>(value);
            i += 2;
        }
        else result += str[i];
    }
    return result;
}

// ----------------- Parse POST Data -----------------
std::map<std::string, std::string> parsePostData() {
    std::map<std::string, std::string> data;
    char* contentLength = std::getenv("CONTENT_LENGTH");
    if (contentLength) {
        int length = std::atoi(contentLength);
        if (length > 0) {
            std::string postData(length, '\0');
            std::cin.read(&postData[0], length);

            std::stringstream ss(postData);
            std::string pair;
            while (std::getline(ss, pair, '&')) {
                size_t pos = pair.find('=');
                if (pos != std::string::npos) {
                    std::string key = pair.substr(0, pos);
                    std::string value = pair.substr(pos + 1);
                    data[key] = urlDecode(value);
                }
            }
        }
    }
    return data;
}

// ----------------- SHA-256 Password Hash -----------------
std::string hashPassword(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(password.c_str()),
        password.length(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    return ss.str();
}

// ----------------- Session Token Generator -----------------
std::string generateSessionToken() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::stringstream ss;
    for (int i = 0; i < 32; i++)
        ss << std::hex << dis(gen);
    return ss.str();
}

// ----------------- Email Validator -----------------
bool isValidEmail(const std::string& email) {
    std::size_t atPos = email.find('@');
    std::size_t dotPos = email.find('.', atPos);
    return (atPos != std::string::npos && dotPos != std::string::npos &&
        atPos > 0 && dotPos > atPos + 1 && dotPos < email.length() - 1);
}
