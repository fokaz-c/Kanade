add_executable(kanade
    src/main.cpp
)

target_link_libraries(kanade PRIVATE
    core
    httplib::httplib
)

add_custom_target(run
    COMMAND $<TARGET_FILE:kanade>
    DEPENDS kanade
    WORKING_DIRECTORY ${CMAKE_PROJECT_SOURCE_DIR}
    USES_TERMINAL
    VERBATIM
)
