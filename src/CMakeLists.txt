
# add the executable
add_executable(DistrubitedTask "main/main.cpp")
target_include_directories(DistrubitedTask PUBLIC
                          "${PROJECT_BINARY_DIR}/src/headers"  "${PROJECT_SOURCE_DIR}/src/headers"
                          ${UWEBSOCKETS_INCLUDE_DIRS}
                          )
target_link_libraries(DistrubitedTask Threads::Threads stdc++fs nlohmann_json::nlohmann_json ${USOCKETS_LIB} ${LIBUV_LIB} ZLIB::ZLIB  absl::any absl::base absl::flags_parse cpr::cpr redis++::redis++_static fmt::fmt spdlog::spdlog spdlog::spdlog_header_only)
target_compile_options(DistrubitedTask PUBLIC -Wall -Wextra -pedantic -Wconversion -Wshadow -O3)


add_executable(Producer "main/producer.cpp")
target_include_directories(Producer PUBLIC
                          "${PROJECT_BINARY_DIR}/src/headers"  "${PROJECT_SOURCE_DIR}/src/headers"
                          )
target_link_libraries(Producer Threads::Threads stdc++fs redis++::redis++_static fmt::fmt spdlog::spdlog spdlog::spdlog_header_only)
target_compile_options(Producer PUBLIC -Wall -Wextra -pedantic -Wconversion -Wshadow -O3)


add_executable(Fetch "main/fetch.cpp")
target_include_directories(Fetch PUBLIC
                          "${PROJECT_BINARY_DIR}/src/headers"  "${PROJECT_SOURCE_DIR}/src/headers"
                          )
target_link_libraries(Fetch Threads::Threads stdc++fs redis++::redis++_static fmt::fmt spdlog::spdlog spdlog::spdlog_header_only)
target_compile_options(Fetch PUBLIC -Wall -Wextra -pedantic -Wconversion -Wshadow -O3)


add_executable(Test "main/test.cpp")
target_compile_options(Test PUBLIC -Wall -Wextra -pedantic -Wconversion -Wshadow -O3)