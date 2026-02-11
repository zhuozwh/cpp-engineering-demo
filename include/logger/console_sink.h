#pragma once

#include "log_sink.h"
#include <mutex>

namespace logger {

class ConsoleSink : public LogSink{
public:

    ConsoleSink() = default;
    ~ConsoleSink() override = default;

    void write(const std::string& formatted_message) override;

private:
    std::mutex mutex_;
};

} // namespace logger
