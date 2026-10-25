#ifndef UTILS_H
#define UTILS_H

#include <stdexcept>
#include <string>
#include <vector>

// Thrown when standard input is closed (Ctrl+D / Ctrl+Z or end of piped input),
// so the program can exit cleanly instead of looping forever.
struct InputClosed : public std::runtime_error
{
    InputClosed() : std::runtime_error("input closed") {}
};

namespace util
{
    std::string trim(const std::string &s);

    // Prompts and reads one full line. Throws InputClosed on end of input.
    std::string readLine(const std::string &prompt);

    // Non-empty text of at most maxBytes bytes.
    std::string readText(const std::string &prompt, std::size_t maxBytes);

    // Whole number in [minValue, maxValue]; re-prompts until valid.
    int readInt(const std::string &prompt, int minValue, int maxValue);

    // Money amount such as 799 or 799.50 (at most 2 decimals); returned as text
    // so it goes to MySQL as an exact DECIMAL, never as a floating-point number.
    std::string readMoney(const std::string &prompt);

    // Basic e-mail shape check: one '@', a dot in the domain, no spaces.
    std::string readEmail(const std::string &prompt);

    // Escapes %, _ and | so user text can be used inside LIKE ... ESCAPE '|'.
    std::string escapeLike(const std::string &s);

    // Prints an aligned text table. Numeric columns are right-aligned.
    void printTable(const std::vector<std::string> &headers,
                    const std::vector<std::vector<std::string>> &rows);
}

#endif
