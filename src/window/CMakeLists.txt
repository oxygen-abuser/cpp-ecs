find_package(SDL3 REQUIRED CONFIG)

if(TARGET SDL3::SDL3-shared)
    set(SDL3_LINK_TARGET SDL3::SDL3-shared)
elseif(TARGET SDL3::SDL3-static)
    set(SDL3_LINK_TARGET SDL3::SDL3-static)
else()
    set(SDL3_LINK_TARGET SDL3::SDL3)
endif()

add_library(window)

target_sources(window
    PUBLIC FILE_SET CXX_MODULES FILES
        window_plugin.cpp
        systems/init.cpp
        systems/poll.cpp
        systems/swap.cpp
        systems/shutdown.cpp
        window.cppm
        sdl.cppm
        types.cppm
)

target_link_libraries(window
    PUBLIC
    ecs
    ${SDL3_LINK_TARGET}
    bgfx
    bimg
    bx
    wayland-egl
)

