file(GLOB sample_sources *.cxx)

foreach(sample_source ${sample_sources})
    file(RELATIVE_PATH filename ${CMAKE_CURRENT_SOURCE_DIR} ${sample_source})
    string(REGEX REPLACE "\\.[^.]*$" "" name ${filename})
    add_executable(${name} ${filename})
    target_link_libraries(${name} log async)
endforeach()
