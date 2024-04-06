file(GLOB log_sources *.cpp)

add_library(log ${log_sources})

target_include_directories(log PUBLIC "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(log PUBLIC pthread fmt)
