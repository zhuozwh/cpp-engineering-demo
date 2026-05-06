#include "config/config.h"

#include <algorithm>
#include <cctype>
#include <fstream>

namespace config {

bool Config::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    // 重新加载配置时，先清空旧配置。
    values_.clear();

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);

        // 跳过空行和注释行。
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // 只处理 key=value 格式。
        std::size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));

        // key 为空的配置项没有意义，直接忽略。
        if (!key.empty()) {
            values_[key] = value;
        }
    }

    return true;
}

std::string Config::get_string(const std::string& key,
                               const std::string& default_value) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
        return default_value;
    }

    return it->second;
}

int Config::get_int(const std::string& key, int default_value) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
        return default_value;
    }

    try {
        return std::stoi(it->second);
    } catch (...) {
        // 配置存在但格式错误时，不让程序崩溃，返回默认值。
        return default_value;
    }
}

bool Config::get_bool(const std::string& key, bool default_value) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
        return default_value;
    }

    std::string value = it->second;

    // 转成小写，兼容 TRUE / True / true 等写法。
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) {
                       return static_cast<char>(std::tolower(c));
                   });

    if (value == "true" || value == "1" || value == "yes" || value == "on") {
        return true;
    }

    if (value == "false" || value == "0" || value == "no" || value == "off") {
        return false;
    }

    return default_value;
}

std::string Config::trim(const std::string& s) {
    std::size_t start = 0;

    // 找到第一个非空白字符。
    while (start < s.size() &&
           std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }

    std::size_t end = s.size();

    // 找到最后一个非空白字符的后一位。
    while (end > start &&
           std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }

    return s.substr(start, end - start);
}

}  // namespace config