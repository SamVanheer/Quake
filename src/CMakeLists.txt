find_package(Threads REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

add_subdirectory(common)
add_subdirectory(stdlib)

add_subdirectory(client_launcher)
add_subdirectory(dedicated_launcher)
add_subdirectory(engine)
