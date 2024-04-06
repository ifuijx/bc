file(GLOB async_sources *.cpp)

add_library(async ${async_sources})

target_include_directories(async PUBLIC "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(async PUBLIC fmt log)
