add_library(core STATIC)

target_sources(core PRIVATE
    src/spotify/spotify_client.cpp
    src/utils/pkce.cpp
    src/utils/load_file.cpp
)

target_include_directories(core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(core
    PUBLIC
        nlohmann_json::nlohmann_json
    PRIVATE
        cpr::cpr
)
