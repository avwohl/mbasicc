#pragma once

#include <variant>
#include <string>
#include <cstdint>
#include <cmath>

namespace mbasic {

// MBASIC variable types
enum class VarType {
    INTEGER,    // 16-bit signed integer (% suffix)
    SINGLE,     // Single-precision float (! suffix, default)
    DOUBLE,     // Double-precision float (# suffix)
    STRING      // String ($ suffix)
};

// Runtime value - can hold any MBASIC type
// Order matters: index 0=INTEGER, 1=SINGLE, 2=DOUBLE, 3=STRING
using Value = std::variant<int16_t, float, double, std::string>;

// Get the VarType of a value
inline VarType get_type(const Value& v) {
    switch (v.index()) {
        case 0: return VarType::INTEGER;
        case 1: return VarType::SINGLE;
        case 2: return VarType::DOUBLE;
        case 3: return VarType::STRING;
        default: return VarType::SINGLE;  // Should never happen
    }
}

// Check if value is numeric
inline bool is_numeric(const Value& v) {
    return v.index() < 3;  // INTEGER, SINGLE, or DOUBLE
}

// Check if value is string
inline bool is_string(const Value& v) {
    return v.index() == 3;
}

// Convert value to double (for numeric operations)
inline double to_number(const Value& v) {
    return std::visit([](auto&& arg) -> double {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::string>) {
            // Type mismatch - should throw, but return 0 for now
            return 0.0;
        } else {
            return static_cast<double>(arg);
        }
    }, v);
}

// Convert value to int16_t
inline int16_t to_integer(const Value& v) {
    double d = to_number(v);
    // MBASIC uses banker's rounding (round half to even)
    if (d >= 32767.5) return 32767;
    if (d <= -32768.5) return -32768;
    // Use rint which respects the current rounding mode (default is round to nearest even)
    return static_cast<int16_t>(std::rint(d));
}

// Convert value to string representation
inline std::string to_string(const Value& v) {
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return arg;
        } else if constexpr (std::is_same_v<T, int16_t>) {
            std::string s = std::to_string(arg);
            // MBASIC adds leading space for positive numbers, trailing space for all
            if (arg >= 0) s = " " + s;
            s += " ";
            return s;
        } else {
            // Float/double formatting
            std::string s;
            double d = static_cast<double>(arg);
            if (d == static_cast<int64_t>(d) && std::abs(d) < 1e10) {
                // Integer value, display without decimal
                s = std::to_string(static_cast<int64_t>(d));
            } else {
                // Use default formatting
                s = std::to_string(d);
                // Remove trailing zeros after decimal
                if (s.find('.') != std::string::npos) {
                    while (s.back() == '0') s.pop_back();
                    if (s.back() == '.') s.pop_back();
                }
            }
            // MBASIC adds leading space for positive numbers, trailing space for all
            if (d >= 0) s = " " + s;
            s += " ";
            return s;
        }
    }, v);
}

// Convert value to boolean (for conditionals)
// 0 = false, non-zero = true, empty string = false
inline bool to_bool(const Value& v) {
    return std::visit([](auto&& arg) -> bool {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return !arg.empty();
        } else {
            return arg != 0;
        }
    }, v);
}

// Coerce a value to a specific type
inline Value coerce_to(const Value& v, VarType target) {
    switch (target) {
        case VarType::INTEGER:
            return to_integer(v);
        case VarType::SINGLE:
            return static_cast<float>(to_number(v));
        case VarType::DOUBLE:
            return to_number(v);
        case VarType::STRING:
            if (is_string(v)) return v;
            // Type mismatch for numeric to string coercion
            return std::string{};
    }
    return v;
}

// Get type suffix character
inline char type_suffix(VarType t) {
    switch (t) {
        case VarType::INTEGER: return '%';
        case VarType::SINGLE: return '!';
        case VarType::DOUBLE: return '#';
        case VarType::STRING: return '$';
    }
    return '!';
}

// Get VarType from suffix character
inline VarType type_from_suffix(char suffix) {
    switch (suffix) {
        case '%': return VarType::INTEGER;
        case '!': return VarType::SINGLE;
        case '#': return VarType::DOUBLE;
        case '$': return VarType::STRING;
        default:  return VarType::SINGLE;  // Default type
    }
}

// Default value for a type
inline Value default_value(VarType t) {
    switch (t) {
        case VarType::INTEGER: return int16_t{0};
        case VarType::SINGLE: return float{0.0f};
        case VarType::DOUBLE: return double{0.0};
        case VarType::STRING: return std::string{};
    }
    return float{0.0f};
}

} // namespace mbasic
