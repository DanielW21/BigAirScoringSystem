add_library(BigAirModules 
    src/calculations.cpp 
    src/displays.cpp
)

target_link_libraries(BigAirModules PUBLIC 
    pico_ssd1306
    pico_stdlib
)

target_include_directories(BigAirModules PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)