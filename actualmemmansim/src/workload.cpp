#include "sim/workload.h"

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <cctype>

namespace sim {

// Utility: trim whitespace
static inline std::string trim(const std::string &s) {
    size_t a = 0;
    while (a < s.size() && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    size_t b = s.size();
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

// Simple CSV split (no quoted fields)
static std::vector<std::string> split_csv_line(const std::string &line) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : line) {
        if (c == ',') {
            out.push_back(trim(cur));
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    out.push_back(trim(cur));
    return out;
}

// Map event name -> priority (lower number = higher priority)
static int event_priority_for_name(const std::string &ename) {
    static const std::unordered_map<std::string, int> map = {
        {"PAGEIN_COMPLETE", 0},
        {"IO_COMPLETE",     0},
        {"WAKEUP",          1},
        {"TIMER",           2},
        {"ACCESS",          3},
        {"MALLOC",          4},
        {"FREE",            4},
        {"PROC_START",      4},
        {"PROC_EXIT",       4},
        {"SLEEP",           4},
        {"IO_START",        4}
    };
    auto it = map.find(ename);
    return (it != map.end()) ? it->second : 5;
}

// Parse decimal or hex integer
static bool parse_u64(const std::string &s, uint64_t &out) {
    try {
        if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
            out = std::stoull(s, nullptr, 16);
        } else {
            out = std::stoull(s, nullptr, 10);
        }
        return true;
    } catch (...) {
        return false;
    }
}

// ================== Workload implementation ==================

Workload::Workload(const std::string &path)
    : path_(path) {}

bool Workload::parse_into(EventQueue &q) {
    std::ifstream ifs(path_);
    if (!ifs) {
        std::cerr << "Workload: failed to open trace file: " << path_ << "\n";
        return false;
    }

    std::string line;
    size_t lineno = 0;

    while (std::getline(ifs, line)) {
        ++lineno;

        std::string raw = line;
        auto comment = line.find('#');
        if (comment != std::string::npos)
            line = line.substr(0, comment);

        line = trim(line);
        if (line.empty()) continue;

        auto toks = split_csv_line(line);
        if (toks.size() < 3) {
            std::cerr << "Workload: malformed line " << lineno << "\n";
            continue;
        }

        uint64_t ts = 0;
        if (!parse_u64(toks[0], ts)) {
            std::cerr << "Workload: invalid timestamp at line " << lineno << "\n";
            continue;
        }

        uint32_t pid = 0;
        try {
            pid = static_cast<uint32_t>(std::stoul(toks[1]));
        } catch (...) {
            std::cerr << "Workload: invalid pid at line " << lineno << "\n";
            continue;
        }

        std::string evname = toks[2];
        std::vector<std::string> args(toks.begin() + 3, toks.end());
        int pri = event_priority_for_name(evname);

        q.push(ts, pri, pid, evname, args, raw);
    }

    return true;
}

} // namespace sim
