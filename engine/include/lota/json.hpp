#pragma once
#include <cctype>
#include <cmath>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace lota {
struct Json {
    enum Type { Null, Bool, Number, String, Array, Object } type = Null;
    double number = 0;
    std::string string;
    std::vector<Json> array;
    std::map<std::string, Json> object;
    const Json &operator[](const std::string &k) const {
        static Json empty;
        auto it = object.find(k);
        return it == object.end() ? empty : it->second;
    }
    const Json &operator[](size_t i) const {
        if (i >= array.size())
            throw std::runtime_error("JSON array index");
        return array[i];
    }
    int integer(int fallback = 0) const {
        return type == Number && number >= std::numeric_limits<int>::min() &&
                       number <= std::numeric_limits<int>::max()
                   ? int(number)
                   : fallback;
    }
    float real(float fallback = 0) const { return type == Number ? float(number) : fallback; }
    bool boolean(bool fallback = false) const { return type == Bool ? number != 0 : fallback; }
    std::string text(std::string fallback = "") const { return type == String ? string : fallback; }
    bool has(const std::string &k) const { return object.count(k); }
    static Json parse(const std::string &s) {
        size_t p = 0;
        auto result = read(s, p, 0);
        space(s, p);
        if (p != s.size())
            throw std::runtime_error("Trailing JSON");
        return result;
    }
    static Json load(const std::string &path) {
        std::ifstream f(path);
        if (!f)
            throw std::runtime_error("Cannot open " + path);
        std::ostringstream b;
        b << f.rdbuf();
        if (b.str().size() > 32 * 1024 * 1024)
            throw std::runtime_error("JSON too large");
        return parse(b.str());
    }

  private:
    static void space(const std::string &s, size_t &p) {
        while (p < s.size() && isspace((unsigned char)s[p]))
            ++p;
    }
    static std::string str(const std::string &s, size_t &p) {
        if (p >= s.size() || s[p++] != '"')
            throw std::runtime_error("Expected string");
        std::string out;
        while (p < s.size()) {
            char c = s[p++];
            if (c == '"')
                return out;
            if ((unsigned char)c < 32)
                throw std::runtime_error("Control in JSON");
            if (c == '\\') {
                if (p >= s.size())
                    break;
                c = s[p++];
                switch (c) {
                case 'n':
                    c = '\n';
                    break;
                case 'r':
                    c = '\r';
                    break;
                case 't':
                    c = '\t';
                    break;
                case 'b':
                    c = '\b';
                    break;
                case 'f':
                    c = '\f';
                    break;
                case '"':
                case '\\':
                case '/':
                    break;
                case 'u': {
                    unsigned code = 0;
                    for (int i = 0; i < 4; ++i) {
                        if (p >= s.size())
                            throw std::runtime_error("Unicode escape");
                        char h = s[p++];
                        int v = h >= '0' && h <= '9'   ? h - '0'
                                : h >= 'a' && h <= 'f' ? h - 'a' + 10
                                : h >= 'A' && h <= 'F' ? h - 'A' + 10
                                                       : -1;
                        if (v < 0)
                            throw std::runtime_error("Unicode escape");
                        code = code * 16 + v;
                    }
                    if (code < 128)
                        out += char(code);
                    else if (code < 2048) {
                        out += char(192 | (code >> 6));
                        out += char(128 | (code & 63));
                    } else {
                        out += char(224 | (code >> 12));
                        out += char(128 | ((code >> 6) & 63));
                        out += char(128 | (code & 63));
                    }
                    continue;
                }
                default:
                    throw std::runtime_error("JSON escape");
                }
            }
            out += c;
        }
        throw std::runtime_error("Unterminated string");
    }
    static Json read(const std::string &s, size_t &p, int depth) {
        if (depth > 48)
            throw std::runtime_error("JSON nesting");
        space(s, p);
        if (p >= s.size())
            throw std::runtime_error("Truncated JSON");
        Json j;
        char c = s[p];
        if (c == '"') {
            j.type = String;
            j.string = str(s, p);
            return j;
        }
        if (c == '{' || c == '[') {
            ++p;
            j.type = c == '{' ? Object : Array;
            space(s, p);
            char end = c == '{' ? '}' : ']';
            if (p < s.size() && s[p] == end) {
                ++p;
                return j;
            }
            for (;;) {
                space(s, p);
                if (c == '{') {
                    std::string key = str(s, p);
                    space(s, p);
                    if (p >= s.size() || s[p++] != ':')
                        throw std::runtime_error("JSON colon");
                    if (j.has(key))
                        throw std::runtime_error("Duplicate JSON key");
                    j.object.emplace(key, read(s, p, depth + 1));
                } else
                    j.array.push_back(read(s, p, depth + 1));
                space(s, p);
                if (p >= s.size())
                    throw std::runtime_error("JSON container");
                if (s[p++] == end)
                    return j;
                if (s[p - 1] != ',')
                    throw std::runtime_error("JSON comma");
            }
        }
        for (auto literal : {std::string("true"), std::string("false"), std::string("null")})
            if (s.compare(p, literal.size(), literal) == 0) {
                p += literal.size();
                j.type = literal == "null" ? Null : Bool;
                j.number = literal == "true";
                return j;
            }
        size_t start = p;
        if (s[p] == '-')
            ++p;
        if (p >= s.size() || !isdigit((unsigned char)s[p]))
            throw std::runtime_error("JSON number");
        if (s[p] == '0')
            ++p;
        else
            while (p < s.size() && isdigit((unsigned char)s[p]))
                ++p;
        if (p < s.size() && s[p] == '.') {
            ++p;
            size_t a = p;
            while (p < s.size() && isdigit((unsigned char)s[p]))
                ++p;
            if (a == p)
                throw std::runtime_error("JSON fraction");
        }
        if (p < s.size() && (s[p] == 'e' || s[p] == 'E')) {
            ++p;
            if (p < s.size() && (s[p] == '+' || s[p] == '-'))
                ++p;
            size_t a = p;
            while (p < s.size() && isdigit((unsigned char)s[p]))
                ++p;
            if (a == p)
                throw std::runtime_error("JSON exponent");
        }
        j.type = Number;
        j.number = std::stod(s.substr(start, p - start));
        if (!std::isfinite(j.number))
            throw std::runtime_error("Nonfinite JSON");
        return j;
    }
};
} // namespace lota
