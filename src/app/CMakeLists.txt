add_library(app)

target_sources(app
    PRIVATE
        app.cpp
    PUBLIC FILE_SET CXX_MODULES FILES
        app.cppm
)

target_link_libraries(app PUBLIC ecs window render)
