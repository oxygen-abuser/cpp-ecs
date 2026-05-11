add_library(render)

target_sources(render
    PUBLIC FILE_SET CXX_MODULES FILES
    render.cppm
    types.cppm
)

target_link_libraries(render
    PUBLIC
    ecs
    bgfx
    bimg
    bx
    wayland-egl
)

