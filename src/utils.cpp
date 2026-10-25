#include "utils.h"

#include <algorithm>
#include <iostream>

using namespace std;

namespace util
{
    string trim(const string &s)
    {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == string::npos)
            return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    string readLine(const string &prompt)
    {
        cout << prompt << flush;
        string line;
        if (!getline(cin, line))
            throw InputClosed();
        return trim(line);
    }

    string readText(const string &prompt, size_t maxBytes)
    {
        while (true)
        {
            string text = readLine(prompt);
            if (text.empty())
            {
                cout << "This field cannot be empty.\n";
                continue;
            }
            if (text.size() > maxBytes)
            {
                cout << "Too long (maximum " << maxBytes << " characters).\n";
                continue;
            }
            return text;
        }
    }

    int readInt(const string &prompt, int minValue, int maxValue)
    {
        while (true)
        {
            string text = readLine(prompt);
            bool valid = !text.empty() && text.size() <= 10;
            size_t start = (valid && text[0] == '-') ? 1 : 0;
            if (start == text.size())
                valid = false;
            for (size_t i = start; valid && i < text.size(); ++i)
                if (text[i] < '0' || text[i] > '9')
                    valid = false;

            if (valid)
            {
                long long value = stoll(text);
                if (value >= minValue && value <= maxValue)
                    return static_cast<int>(value);
            }
            cout << "Please enter a whole number between " << minValue
                 << " and " << maxValue << ".\n";
        }
    }

    string readMoney(const string &prompt)
    {
        while (true)
        {
            string text = readLine(prompt);
            size_t dot = text.find('.');
            string whole = (dot == string::npos) ? text : text.substr(0, dot);
            string frac = (dot == string::npos) ? "" : text.substr(dot + 1);

            auto allDigits = [](const string &s)
            {
                return all_of(s.begin(), s.end(), [](char c)
                              { return c >= '0' && c <= '9'; });
            };

            bool valid = !whole.empty() && whole.size() <= 8 && allDigits(whole) &&
                         frac.size() <= 2 && allDigits(frac) &&
                         (dot == string::npos || !frac.empty());
            if (valid)
                return text;
            cout << "Enter a valid amount, e.g. 799 or 799.50.\n";
        }
    }

    string readEmail(const string &prompt)
    {
        while (true)
        {
            string email = readText(prompt, 100);
            size_t at = email.find('@');
            bool valid = at != string::npos && at > 0 &&
                         email.find('@', at + 1) == string::npos &&
                         email.find(' ') == string::npos;
            if (valid)
            {
                size_t dot = email.find('.', at);
                valid = dot != string::npos && dot > at + 1 && dot + 1 < email.size();
            }
            if (valid)
                return email;
            cout << "Please enter a valid e-mail address.\n";
        }
    }

    string escapeLike(const string &s)
    {
        string out;
        for (char c : s)
        {
            if (c == '%' || c == '_' || c == '|')
                out += '|';
            out += c;
        }
        return out;
    }

    // Number of characters (UTF-8 code points), so accented names align.
    static size_t displayWidth(const string &s)
    {
        size_t n = 0;
        for (unsigned char c : s)
            if ((c & 0xC0) != 0x80)
                ++n;
        return n;
    }

    static bool looksNumeric(const string &s)
    {
        if (s.empty())
            return false;
        size_t i = (s[0] == '-') ? 1 : 0;
        bool digits = false, dot = false;
        for (; i < s.size(); ++i)
        {
            if (s[i] >= '0' && s[i] <= '9')
                digits = true;
            else if (s[i] == '.' && !dot)
                dot = true;
            else
                return false;
        }
        return digits;
    }

    void printTable(const vector<string> &headers, const vector<vector<string>> &rows)
    {
        if (rows.empty())
        {
            cout << "(no records)\n";
            return;
        }

        size_t cols = headers.size();
        vector<size_t> width(cols);
        vector<bool> numeric(cols, true);

        for (size_t c = 0; c < cols; ++c)
        {
            width[c] = displayWidth(headers[c]);
            for (const auto &row : rows)
            {
                width[c] = max(width[c], displayWidth(row[c]));
                if (!looksNumeric(row[c]))
                    numeric[c] = false;
            }
        }

        auto cell = [&](const string &text, size_t c, bool last)
        {
            size_t pad = width[c] - displayWidth(text);
            if (numeric[c])
                cout << string(pad, ' ') << text;
            else
            {
                cout << text;
                if (!last)
                    cout << string(pad, ' ');
            }
            if (!last)
                cout << "  ";
        };

        cout << '\n';
        for (size_t c = 0; c < cols; ++c)
            cell(headers[c], c, c + 1 == cols);
        cout << '\n';
        for (size_t c = 0; c < cols; ++c)
        {
            cout << string(width[c], '-');
            if (c + 1 != cols)
                cout << "  ";
        }
        cout << '\n';
        for (const auto &row : rows)
        {
            for (size_t c = 0; c < cols; ++c)
                cell(row[c], c, c + 1 == cols);
            cout << '\n';
        }
    }
}
