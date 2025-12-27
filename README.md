# cpp_service_framework

A C++ service framework project built for campus recruitment.

---

## Current Status

- Logger module v0 completed
- Supports log levels and logging macros
- Project builds with CMake and runs correctly

---

## Logger Module (v0)

### Features

- Singleton logger instance
- Log levels: DEBUG / INFO / WARN / ERROR
- Logging macros with file / line / function information

### Example

```cpp
#include "logger/logger.h"

int main() {
    LOG_INFO("Server starting");
    LOG_WARN("This is a warning");
    LOG_ERROR("Something went wrong");
    return 0;
}
```

### Build & RUN

```bash
mkdir build && cd build
cmake ..
make
./cpp_service_framework
```

### Roadmap

- Logger v0: basic logging (finished)
- Logger v1: thread safety and timestamp
- Logger v2: file sink
- Network server integration
