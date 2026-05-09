include(FetchContent)

# ── bgfx (via the unofficial CMake wrapper) ──────────────────────────────────
FetchContent_Declare(
    bgfx
    GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
    GIT_TAG        master          # pin to a commit hash in production
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(bgfx)

# ── Slang ─────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    slang
    GIT_REPOSITORY https://github.com/shader-slang/slang.git
    GIT_TAG        master          # or a specific release tag e.g. v2025.6.3
    GIT_SHALLOW    TRUE
)

# Disable everything you don't need to keep build times reasonable
set(SLANG_ENABLE_TESTS      OFF CACHE BOOL "" FORCE)
set(SLANG_ENABLE_EXAMPLES   OFF CACHE BOOL "" FORCE)
set(SLANG_ENABLE_GFX        OFF CACHE BOOL "" FORCE)  # bgfx is your GFX layer


set(SLANG_USE_SYSTEM_SPIRV_TOOLS  ON CACHE BOOL "" FORCE)
set(SLANG_USE_SYSTEM_GLSLANG      ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(slang)

FetchContent_Declare(
    EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG        main          # or pin to e.g. v3.13.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(EnTT)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
