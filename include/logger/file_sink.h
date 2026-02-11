#pragma once

#include "log_sink.h"
#include <fstream>
#include <mutex>

namespace logger {

class FileSink : public LogSink {
public:
    explicit FileSink(const std::string& filename);
    ~FileSink() override = default;

    void write(const std::string& formatted_message) override;

private:
    std::ofstream file_;
    std::mutex mutex_;
};

} // namespace logger
