#include "ecs.hpp"
#include <gtest/gtest.h>

// components
struct position {
  float x, y, z;
};
struct velocity {
  float dx, dy, dz;
};
struct health {
  int value;
};
struct tag_enemy {};
struct tag_player {};
struct tag_camera {};

// ── basic iteration
// ───────────────────────────────────────────────────────────

TEST(Query, IteratesAllMatchingEntities) {
  auto world = ecs::world::world::create();

  auto e1 = world.create_entity();
  auto e2 = world.create_entity();
  auto e3 = world.create_entity();
  world.add_component(e1, position{.x = 0.f, .y = 0.f, .z = 0.f});
  world.add_component(e2, position{.x = 0.f, .y = 0.f, .z = 0.f});
  world.add_component(e3, position{.x = 0.f, .y = 0.f, .z = 0.f});

  auto commands = ecs::commands::commands{};
  auto context = ecs::system::system_context{world, commands};

  int count = 0;
  auto q = ecs::query::query<position>::fetch(context);
  q.for_each([&count](position &) { ++count; });

  EXPECT_EQ(count, 3);
}

TEST(Query, SkipsEntitiesWithoutComponent) {
  auto world = ecs::world::world::create();

  auto e1 = world.create_entity();
  auto e2 = world.create_entity();
  world.add_component(e1, position{.x = 0.f, .y = 0.f, .z = 0.f});
  // e2 has no position

  auto commands = ecs::commands::commands{};
  auto context = ecs::system::system_context{world, commands};

  int count = 0;
  auto q = ecs::query::query<position>::fetch(context);
  q.for_each([&count](position &) { ++count; });

  EXPECT_EQ(count, 1);
}

TEST(Query, MultipleComponents) {
  auto world = ecs::world::world::create();

  auto e1 = world.create_entity();
  auto e2 = world.create_entity();
  auto e3 = world.create_entity();
  world.add_component(e1, position{.x = 1.f, .y = 0.f, .z = 0.f});
  world.add_component(e1, velocity{.dx = 1.f, .dy = 0.f, .dz = 0.f});
  world.add_component(e2, position{.x = 2.f, .y = 0.f, .z = 0.f});
  // e2 has no velocity
  world.add_component(e3, velocity{.dx = 3.f, .dy = 0.f, .dz = 0.f});
  // e3 has no position
  auto commands = ecs::commands::commands{};
  auto context = ecs::system::system_context{world, commands};

  int count = 0;
  auto q = ecs::query::query<position, velocity>::fetch(context);
  q.for_each([&count](position &, velocity &) { ++count; });

  // only e1 has both
  EXPECT_EQ(count, 1);
}

// ── mutation
// ──────────────────────────────────────────────────────────────────

TEST(Query, MutatesComponent) {
  auto world = ecs::world::world::create();

  auto e = world.create_entity();
  world.add_component(e, position{.x = 0.f, .y = 0.f, .z = 0.f});
  world.add_component(e, velocity{.dx = 5.f, .dy = 0.f, .dz = 0.f});

  auto commands = ecs::commands::commands{};
  auto context = ecs::system::system_context{world, commands};

  auto q = ecs::query::query<position, const velocity>::fetch(context);
  q.for_each([](position &p, const velocity &v) { p.x += v.dx; });

  EXPECT_FLOAT_EQ(world.get_component<position>(e).x, 5.f);
}

TEST(Query, ConstDoesNotMutate) {
  auto world = ecs::world::world::create();

  auto e = world.create_entity();
  world.add_component(e, velocity{.dx = 5.f, .dy = 0.f, .dz = 0.f});

  auto commands = ecs::commands::commands{};
  auto context = ecs::system::system_context{world, commands};

  // just verifying const access compiles and reads correctly
  auto q = ecs::query::query<const velocity>::fetch(context);
  float read_dx = 0.f;
  q.for_each([&read_dx](const velocity &v) { read_dx = v.dx; });

  EXPECT_FLOAT_EQ(read_dx, 5.f);
}

// ── entity
// ────────────────────────────────────────────────────────────────────

TEST(Query, EntityParamProvided) {
  auto world = ecs::world::world::create();

  auto e = world.create_entity();
  world.add_component(e, position{.x = 0.f, .y = 0.f, .z = 0.f});

  auto commands = ecs::commands::commands{};
  auto context = ecs::system::system_context{world, commands};

  ecs::entity::entity captured = ecs::entity::null;
  auto q = ecs::query::query<ecs::entity::entity, position>::fetch(context);
  q.for_each([&captured](ecs::entity::entity ent, position &) { captured = ent; });

  EXPECT_EQ(captured, e);
}

TEST(Query, EntityIsUniquePerEntity) {
  auto world = ecs::world::world::create();

  auto e1 = world.create_entity();
  auto e2 = world.create_entity();
  world.add_component(e1, position{.x = 0.f, .y = 0.f, .z = 0.f});
  world.add_component(e2, position{.x = 0.f, .y = 0.f, .z = 0.f});

  auto commands = ecs::commands::commands{};
  auto context = ecs::system::system_context{world, commands};

  std::vector<ecs::entity::entity> entities;
  auto q = ecs::query::query<ecs::entity::entity, position>::fetch(context);
  q.for_each(
      [&entities](ecs::entity::entity ent, position &) { entities.push_back(ent); });

  EXPECT_EQ(entities.size(), 2);
  EXPECT_NE(entities[0], entities[1]);
}

// ── with filter
// ───────────────────────────────────────────────────────────────

TEST(QueryFilter, WithMatchesOnly) {
  auto world = ecs::world::world::create();

  auto e1 = world.create_entity();
  auto e2 = world.create_entity();
  auto e3 = world.create_entity();
  world.add_component(e1, position{.x = 1.f, .y = 0.f, .z = 0.f});
  world.add_component(e1, tag_player{});
  world.add_component(e2, position{.x = 2.f, .y = 0.f, .z = 0.f});
  world.add_component(e2, tag_player{});
  world.add_component(e3, position{.x = 3.f, .y = 0.f, .z = 0.f});
  // e3 has no tag_player

  auto commands = ecs::commands::commands{};
  auto context = ecs::system::system_context{world, commands};

  int count = 0;
  auto q =
      ecs::query::query<position, ecs::query::with<tag_player>>::fetch(context);
  q.for_each([&count](position &) { ++count; });

  EXPECT_EQ(count, 2);
}

TEST(QueryFilter, WithNotPassedToLambda) {
  auto world = ecs::world::world::create();

  auto e = world.create_entity();
  world.add_component(e, position{.x = 1.f, .y = 0.f, .z = 0.f});
  world.add_component(e, tag_player{});

  auto commands = ecs::commands::commands{};
  auto context = ecs::system::system_context{world, commands};

  // lambda only receives position, not tag_player
  auto q =
      ecs::query::query<position, ecs::query::with<tag_player>>::fetch(context);
  q.for_each([](position &p) { p.x = 99.f; });

  EXPECT_FLOAT_EQ(world.get_component<position>(e).x, 99.f);
}

TEST(QueryFilter, WithMultipleTypes) {
  auto world = ecs::world::world::create();

  auto e1 = world.create_entity();
  auto e2 = world.create_entity();
  auto e3 = world.create_entity();
  world.add_component(e1, position{.x = 1.f, .y = 0.f, .z = 0.f});
  world.add_component(e1, tag_player{});
  world.add_component(e1, tag_camera{});
  world.add_component(e2, position{.x = 2.f, .y = 0.f, .z = 0.f});
  world.add_component(e2, tag_player{});
  // e2 missing tag_camera
  world.add_component(e3, position{.x = 3.f, .y = 0.f, .z = 0.f});
  // e3 missing both

  auto commands = ecs::commands::commands{};
  auto context = ecs::system::system_context{world, commands};

  int count = 0;
  auto q = ecs::query::query<
      position, ecs::query::with<tag_player, tag_camera>>::fetch(context);
  q.for_each([&count](position &) { ++count; });

  // only e1 has both tags
  EXPECT_EQ(count, 1);
}

// ── without filter
// ────────────────────────────────────────────────────────────

TEST(QueryFilter, WithoutExcludes) {
  auto world = ecs::world::world::create();

  auto e1 = world.create_entity();
  auto e2 = world.create_entity();
  auto e3 = world.create_entity();
  world.add_component(e1, position{.x = 1.f, .y = 0.f, .z = 0.f});
  world.add_component(e2, position{.x = 2.f, .y = 0.f, .z = 0.f});
  world.add_component(e2, tag_enemy{});
  world.add_component(e3, position{.x = 3.f, .y = 0.f, .z = 0.f});
  world.add_component(e3, tag_enemy{});

  auto commands = ecs::commands::commands{};
  auto context = ecs::system::system_context{world, commands};

  int count = 0;
  auto q = ecs::query::query<position, ecs::query::without<tag_enemy>>::fetch(
      context);
  q.for_each([&count](position &) { ++count; });

  EXPECT_EQ(count, 1);
}

TEST(QueryFilter, WithoutMultipleTypes) {
  auto world = ecs::world::world::create();

  auto e1 = world.create_entity(); // neither tag — should match
  auto e2 = world.create_entity(); // only enemy — excluded
  auto e3 = world.create_entity(); // only camera — excluded
  auto e4 = world.create_entity(); // both — excluded
  world.add_component(e1, position{.x = 1.f, .y = 0.f, .z = 0.f});
  world.add_component(e2, position{.x = 2.f, .y = 0.f, .z = 0.f});
  world.add_component(e2, tag_enemy{});
  world.add_component(e3, position{.x = 3.f, .y = 0.f, .z = 0.f});
  world.add_component(e3, tag_camera{});
  world.add_component(e4, position{.x = 4.f, .y = 0.f, .z = 0.f});
  world.add_component(e4, tag_enemy{});
  world.add_component(e4, tag_camera{});

  auto commands = ecs::commands::commands{};
  auto context = ecs::system::system_context{world, commands};

  int count = 0;
  auto q = ecs::query::query<
      position, ecs::query::without<tag_enemy, tag_camera>>::fetch(context);
  q.for_each([&count](position &) { ++count; });

  EXPECT_EQ(count, 1);
}

// ── with + without combined
// ───────────────────────────────────────────────────

TEST(QueryFilter, FilterWithAndWithoutCombined) {
  auto world = ecs::world::world::create();

  auto e1 = world.create_entity(); // player, no enemy — match
  auto e2 = world.create_entity(); // player + enemy — excluded
  auto e3 = world.create_entity(); // no player — excluded
  world.add_component(e1, position{.x = 1.f, .y = 0.f, .z = 0.f});
  world.add_component(e1, tag_player{});
  world.add_component(e2, position{.x = 2.f, .y = 0.f, .z = 0.f});
  world.add_component(e2, tag_player{});
  world.add_component(e2, tag_enemy{});
  world.add_component(e3, position{.x = 3.f, .y = 0.f, .z = 0.f});

  auto commands = ecs::commands::commands{};
  auto context = ecs::system::system_context{world, commands};

  int count = 0;
  auto q = ecs::query::query<position, ecs::query::with<tag_player>,
                             ecs::query::without<tag_enemy>>::fetch(context);
  q.for_each([&count](position &) { ++count; });

  EXPECT_EQ(count, 1);
}

TEST(QueryFilter, EntityWithFilters) {
  auto world = ecs::world::world::create();

  auto e1 = world.create_entity();
  auto e2 = world.create_entity();
  world.add_component(e1, position{.x = 1.f, .y = 0.f, .z = 0.f});
  world.add_component(e1, tag_player{});
  world.add_component(e2, position{.x = 2.f, .y = 0.f, .z = 0.f});
  // e2 no tag_player
  auto commands = ecs::commands::commands{};
  auto context = ecs::system::system_context{world, commands};

  ecs::entity::entity captured = ecs::entity::null;
  auto q = ecs::query::query<ecs::entity::entity, position,
                             ecs::query::with<tag_player>>::fetch(context);
  q.for_each([&captured](ecs::entity::entity ent, position &) { captured = ent; });

  EXPECT_EQ(captured, e1);
}

TEST(QueryFilter, WithThreeTypes) {
    auto world = ecs::world::world::create();

    auto e1 = world.create_entity(); // all three — match
    auto e2 = world.create_entity(); // missing tag_camera — excluded
    auto e3 = world.create_entity(); // missing tag_enemy — excluded
    auto e4 = world.create_entity(); // missing tag_player — excluded

    world.add_component(e1, position{ .x = 1.f, .y = 0.f, .z = 0.f });
    world.add_component(e1, tag_player{});
    world.add_component(e1, tag_enemy{});
    world.add_component(e1, tag_camera{});

    world.add_component(e2, position{ .x = 2.f, .y = 0.f, .z = 0.f });
    world.add_component(e2, tag_player{});
    world.add_component(e2, tag_enemy{});

    world.add_component(e3, position{ .x = 3.f, .y = 0.f, .z = 0.f });
    world.add_component(e3, tag_player{});
    world.add_component(e3, tag_camera{});

    world.add_component(e4, position{ .x = 4.f, .y = 0.f, .z = 0.f });
    world.add_component(e4, tag_enemy{});
    world.add_component(e4, tag_camera{});

    auto commands = ecs::commands::commands{};
    auto context = ecs::system::system_context{ world, commands };

    int count = 0;
    auto q = ecs::query::query<position,
        ecs::query::with<tag_player, tag_enemy, tag_camera>>::fetch(context);
    q.for_each([&count](position&) { ++count; });

    EXPECT_EQ(count, 1);
}

TEST(QueryFilter, WithoutThreeTypes) {
    auto world = ecs::world::world::create();

    auto e1 = world.create_entity(); // none — match
    auto e2 = world.create_entity(); // only player — excluded
    auto e3 = world.create_entity(); // only enemy — excluded
    auto e4 = world.create_entity(); // only camera — excluded
    auto e5 = world.create_entity(); // player + enemy — excluded
    auto e6 = world.create_entity(); // all three — excluded

    world.add_component(e1, position{ .x = 1.f, .y = 0.f, .z = 0.f });

    world.add_component(e2, position{ .x = 2.f, .y = 0.f, .z = 0.f });
    world.add_component(e2, tag_player{});

    world.add_component(e3, position{ .x = 3.f, .y = 0.f, .z = 0.f });
    world.add_component(e3, tag_enemy{});

    world.add_component(e4, position{ .x = 4.f, .y = 0.f, .z = 0.f });
    world.add_component(e4, tag_camera{});

    world.add_component(e5, position{ .x = 5.f, .y = 0.f, .z = 0.f });
    world.add_component(e5, tag_player{});
    world.add_component(e5, tag_enemy{});

    world.add_component(e6, position{ .x = 6.f, .y = 0.f, .z = 0.f });
    world.add_component(e6, tag_player{});
    world.add_component(e6, tag_enemy{});
    world.add_component(e6, tag_camera{});

    auto commands = ecs::commands::commands{};
    auto context = ecs::system::system_context{ world, commands };

    int count = 0;
    auto q = ecs::query::query<position,
        ecs::query::without<tag_player, tag_enemy, tag_camera>>::fetch(context);
    q.for_each([&count](position&) { ++count; });

    EXPECT_EQ(count, 1);
}

TEST(QueryFilter, WithTwoWithoutTwo) {
    auto world = ecs::world::world::create();

    auto e1 = world.create_entity(); // has player+camera, no enemy+health — match
    auto e2 = world.create_entity(); // has player+camera+enemy — excluded
    auto e3 = world.create_entity(); // has player+camera+health — excluded
    auto e4 = world.create_entity(); // has player only — excluded (no camera)
    auto e5 = world.create_entity(); // has camera only — excluded (no player)

    world.add_component(e1, position{ .x = 1.f, .y = 0.f, .z = 0.f });
    world.add_component(e1, tag_player{});
    world.add_component(e1, tag_camera{});

    world.add_component(e2, position{ .x = 2.f, .y = 0.f, .z = 0.f });
    world.add_component(e2, tag_player{});
    world.add_component(e2, tag_camera{});
    world.add_component(e2, tag_enemy{});

    world.add_component(e3, position{ .x = 3.f, .y = 0.f, .z = 0.f });
    world.add_component(e3, tag_player{});
    world.add_component(e3, tag_camera{});
    world.add_component(e3, health{ .value = 1 });

    world.add_component(e4, position{ .x = 4.f, .y = 0.f, .z = 0.f });
    world.add_component(e4, tag_player{});

    world.add_component(e5, position{ .x = 5.f, .y = 0.f, .z = 0.f });
    world.add_component(e5, tag_camera{});

    auto commands = ecs::commands::commands{};
    auto context = ecs::system::system_context{ world, commands };

    int count = 0;
    auto q = ecs::query::query<position,
        ecs::query::with<tag_player, tag_camera>,
        ecs::query::without<tag_enemy, health>>::fetch(context);
    q.for_each([&count](position&) { ++count; });

    EXPECT_EQ(count, 1);
}
