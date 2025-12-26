#include "logger/logger.h"

int main() {
    LOG_INFO("Server starting...");
    LOG_WARN("This is a warning");
    LOG_ERROR("Something went wrong");

    return 0;
}

