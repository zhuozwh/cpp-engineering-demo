#pragma once

#include <string>
#include <unordered_map>

namespace config {

class Config {
public:
    // 从指定文件加载配置。
    // 成功返回 true，文件打开失败返回 false。
    bool load(const std::string& filename);

    // 获取字符串配置。
    // 如果 key 不存在，返回 default_value。
    std::string get_string(const std::string& key,
                           const std::string& default_value = "") const;

    // 获取整数配置。
    // 如果 key 不存在，或者 value 不能转换为 int，返回 default_value。
    int get_int(const std::string& key,
                int default_value = 0) const;

    // 获取布尔配置。
    // 支持 true/false、1/0、yes/no、on/off。
    // 如果 key 不存在或无法识别，返回 default_value。
    bool get_bool(const std::string& key,
                  bool default_value = false) const;

private:
    // 去掉字符串首尾空白字符，方便解析 "key = value" 这种格式。
    static std::string trim(const std::string& s);

private:
    // 保存配置项。
    // 例如：values_["server.port"] = "8080"
    std::unordered_map<std::string, std::string> values_;
};

}  // namespace config