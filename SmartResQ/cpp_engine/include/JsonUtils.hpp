#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <cmath>

namespace smartresq {

/**
 * @brief Lightweight, zero-dependency JSON Value representation for IPC.
 */
struct JsonValue {
    enum Type { Null, Bool, Number, String, Array, Object } type{Null};
    std::string str_val;
    double num_val{0.0};
    bool bool_val{false};
    std::vector<JsonValue> arr_val;
    std::unordered_map<std::string, JsonValue> obj_val;

    // Helpers
    bool isNull() const noexcept { return type == Null; }
    bool isString() const noexcept { return type == String; }
    bool isNumber() const noexcept { return type == Number; }
    bool isBool() const noexcept { return type == Bool; }
    bool isArray() const noexcept { return type == Array; }
    bool isObject() const noexcept { return type == Object; }

    const JsonValue* get(const std::string& key) const {
        if (type != Object) return nullptr;
        auto it = obj_val.find(key);
        return (it != obj_val.end()) ? &it->second : nullptr;
    }

    std::string getString(const std::string& key, const std::string& default_val = "") const {
        const auto* ptr = get(key);
        return (ptr && ptr->isString()) ? ptr->str_val : default_val;
    }

    double getNumber(const std::string& key, double default_val = 0.0) const {
        const auto* ptr = get(key);
        return (ptr && ptr->isNumber()) ? ptr->num_val : default_val;
    }

    bool getBool(const std::string& key, bool default_val = false) const {
        const auto* ptr = get(key);
        return (ptr && ptr->isBool()) ? ptr->bool_val : default_val;
    }

    std::vector<std::string> getStringArray(const std::string& key) const {
        std::vector<std::string> res;
        const auto* ptr = get(key);
        if (ptr && ptr->isArray()) {
            for (const auto& item : ptr->arr_val) {
                if (item.isString()) res.push_back(item.str_val);
            }
        }
        return res;
    }

    std::string serialize() const {
        std::ostringstream ss;
        serializeInternal(ss);
        return ss.str();
    }

private:
    void serializeInternal(std::ostream& os) const {
        switch (type) {
            case Null:
                os << "null";
                break;
            case Bool:
                os << (bool_val ? "true" : "false");
                break;
            case Number:
                if (std::isinf(num_val) || std::isnan(num_val)) {
                    os << "-1.0";
                } else if (num_val == static_cast<long long>(num_val)) {
                    os << static_cast<long long>(num_val);
                } else {
                    os << std::fixed << std::setprecision(4) << num_val;
                }
                break;
            case String:
                os << '"';
                for (char c : str_val) {
                    if (c == '"') os << "\\\"";
                    else if (c == '\\') os << "\\\\";
                    else if (c == '\n') os << "\\n";
                    else if (c == '\t') os << "\\t";
                    else os << c;
                }
                os << '"';
                break;
            case Array:
                os << '[';
                for (size_t i = 0; i < arr_val.size(); ++i) {
                    arr_val[i].serializeInternal(os);
                    if (i + 1 < arr_val.size()) os << ", ";
                }
                os << ']';
                break;
            case Object:
                os << '{';
                size_t idx = 0;
                for (const auto& [k, v] : obj_val) {
                    os << '"' << k << "\": ";
                    v.serializeInternal(os);
                    if (++idx < obj_val.size()) os << ", ";
                }
                os << '}';
                break;
        }
    }
};

class JsonParser {
public:
    static JsonValue parse(const std::string& json_str) {
        size_t pos = 0;
        skipWhitespace(json_str, pos);
        return parseValue(json_str, pos);
    }

private:
    static void skipWhitespace(const std::string& s, size_t& pos) {
        while (pos < s.size() && (std::isspace(s[pos]) || s[pos] == '\r' || s[pos] == '\n')) {
            ++pos;
        }
    }

    static JsonValue parseValue(const std::string& s, size_t& pos) {
        skipWhitespace(s, pos);
        if (pos >= s.size()) return JsonValue{};

        char c = s[pos];
        if (c == '{') return parseObject(s, pos);
        if (c == '[') return parseArray(s, pos);
        if (c == '"') return parseString(s, pos);
        if (std::isdigit(c) || c == '-') return parseNumber(s, pos);
        if (std::isalpha(c)) return parseKeyword(s, pos);

        return JsonValue{};
    }

    static JsonValue parseObject(const std::string& s, size_t& pos) {
        JsonValue val;
        val.type = JsonValue::Object;
        ++pos; // consume '{'

        while (pos < s.size()) {
            skipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == '}') {
                ++pos;
                break;
            }
            if (pos < s.size() && s[pos] == ',') {
                ++pos;
                skipWhitespace(s, pos);
            }

            if (pos >= s.size() || s[pos] != '"') break;
            JsonValue key = parseString(s, pos);
            skipWhitespace(s, pos);

            if (pos >= s.size() || s[pos] != ':') break;
            ++pos; // consume ':'

            JsonValue child = parseValue(s, pos);
            val.obj_val[key.str_val] = std::move(child);
        }
        return val;
    }

    static JsonValue parseArray(const std::string& s, size_t& pos) {
        JsonValue val;
        val.type = JsonValue::Array;
        ++pos; // consume '['

        while (pos < s.size()) {
            skipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == ']') {
                ++pos;
                break;
            }
            if (pos < s.size() && s[pos] == ',') {
                ++pos;
                skipWhitespace(s, pos);
            }

            JsonValue child = parseValue(s, pos);
            val.arr_val.push_back(std::move(child));
        }
        return val;
    }

    static JsonValue parseString(const std::string& s, size_t& pos) {
        JsonValue val;
        val.type = JsonValue::String;
        ++pos; // consume opening '"'

        std::string res;
        while (pos < s.size()) {
            char c = s[pos++];
            if (c == '"') break;
            if (c == '\\' && pos < s.size()) {
                char esc = s[pos++];
                if (esc == '"') res.push_back('"');
                else if (esc == '\\') res.push_back('\\');
                else if (esc == 'n') res.push_back('\n');
                else if (esc == 't') res.push_back('\t');
                else res.push_back(esc);
            } else {
                res.push_back(c);
            }
        }
        val.str_val = std::move(res);
        return val;
    }

    static JsonValue parseNumber(const std::string& s, size_t& pos) {
        JsonValue val;
        val.type = JsonValue::Number;
        size_t start = pos;
        if (s[pos] == '-') ++pos;
        while (pos < s.size() && std::isdigit(s[pos])) ++pos;
        if (pos < s.size() && s[pos] == '.') {
            ++pos;
            while (pos < s.size() && std::isdigit(s[pos])) ++pos;
        }
        if (pos < s.size() && (s[pos] == 'e' || s[pos] == 'E')) {
            ++pos;
            if (pos < s.size() && (s[pos] == '+' || s[pos] == '-')) ++pos;
            while (pos < s.size() && std::isdigit(s[pos])) ++pos;
        }

        std::string sub = s.substr(start, pos - start);
        val.num_val = std::stod(sub);
        return val;
    }

    static JsonValue parseKeyword(const std::string& s, size_t& pos) {
        JsonValue val;
        size_t start = pos;
        while (pos < s.size() && std::isalpha(s[pos])) ++pos;
        std::string sub = s.substr(start, pos - start);
        if (sub == "true") {
            val.type = JsonValue::Bool;
            val.bool_val = true;
        } else if (sub == "false") {
            val.type = JsonValue::Bool;
            val.bool_val = false;
        } else {
            val.type = JsonValue::Null;
        }
        return val;
    }
};

} // namespace smartresq
