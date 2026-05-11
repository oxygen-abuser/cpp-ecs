set(target engine)

add_subdirectory(vendor)

add_subdirectory(ecs)
add_subdirectory(app)
add_subdirectory(window)
add_subdirectory(render)

add_executable(${target} main.cpp)


target_link_libraries(${target} PRIVATE
    bgfx
    bimg
    bx
    slang
    app
)

