set(target engine)

add_subdirectory(vendor)
add_subdirectory(ecs)

add_executable(${target} main.cpp)

target_link_libraries(${target} PRIVATE
    bgfx
    bimg
    bx
    slang
    ecs
)
