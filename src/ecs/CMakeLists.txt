add_library(ecs)

target_sources(ecs
  PRIVATE
  src/ecs.cpp
  src/ecs-resource.cpp
  src/ecs-world.cpp
  src/ecs-system.cpp
  src/ecs-system-traits.cpp
  src/ecs-schedulable.cpp
  src/ecs-query.cpp
  src/ecs-events.cpp
  src/ecs-system-context.cpp
  src/ecs-app.cpp
  tests/query-compile-test.cpp
  tests/system-compile-test.cpp
  PUBLIC FILE_SET CXX_MODULES FILES
  modules/ecs.cppm
)

target_include_directories(ecs
    PRIVATE ./include
)

target_link_libraries(ecs PRIVATE EnTT::EnTT)

enable_testing()

add_executable(ecs_tests
    tests/test_world.cpp
    tests/test_query.cpp
    tests/test_system.cpp
    tests/test_commands.cpp
    tests/test_app.cpp
    tests/test_schedulable.cpp
    tests/test_resources.cpp
    tests/test_graph.cpp
)

target_include_directories(ecs_tests PRIVATE ./include)
target_link_libraries(ecs_tests PRIVATE
    ecs
    GTest::gtest_main
    EnTT::EnTT
)

include(GoogleTest)
add_test(NAME ecs_tests COMMAND ecs_tests)
