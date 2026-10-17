cmake_minimum_required(VERSION 3.16)
project(edge_telemetry C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

add_executable(edge-telemetry
    src/main.c
    src/config.c
    src/telemetry.c
    src/metrics_cpu.c
    src/metrics_mem.c
    src/metrics_temp.c
    src/http_post.c
)
target_compile_definitions(edge-telemetry PRIVATE _POSIX_C_SOURCE=200809L)
find_package(Threads REQUIRED)
target_link_libraries(edge-telemetry PRIVATE Threads::Threads)

install(TARGETS edge-telemetry RUNTIME DESTINATION bin)
install(FILES systemd/edge-telemetry.service DESTINATION /etc/systemd/system)
