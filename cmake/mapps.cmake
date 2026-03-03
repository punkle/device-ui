# Meshtastic app runtime (fetches Berry internally)
message(STATUS "Fetching mapps ...")
FetchContent_Declare(
    mapps
    GIT_REPOSITORY "https://github.com/punkle/mapps"
    GIT_TAG "main"
)
FetchContent_MakeAvailable(mapps)
