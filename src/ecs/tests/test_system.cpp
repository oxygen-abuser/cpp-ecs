#include "ecs.hpp"
#include <gtest/gtest.h>

// resources
struct counter_res {
  int value = 0;
};
struct delta_res {
  float value = 0.016f;
};

// components
struct position {
  float x, y, z;
};
struct velocity {
  float dx, dy, dz;
};

// ── scheduler lifecycle tests
// ─────────────────────────────────────────────────

TEST(Scheduler, Create) {
  auto sched = ecs::system::scheduler::create();
  auto world = ecs::world::world::create();
  // should not crash
  sched.init(world);
  sched.update(world);
  sched.shutdown(world);
}

// ── bind point tests
// ──────────────────────────────────────────────────────────

TEST(Scheduler, InitRunsOnce) {
  auto world = ecs::world::world::create();
  world.insert_resource(counter_res{});

  auto sched = ecs::system::scheduler::create();
  sched.add_system(
      ecs::system::system_bind_point::init,
      [](ecs::resource::resource<counter_res> c) { c->value += 1; });

  sched.init(world);
  EXPECT_EQ(world.resource<counter_res>().value, 1);

  // calling init again should run again (it's the caller's responsibility)
  sched.init(world);
  EXPECT_EQ(world.resource<counter_res>().value, 2);
}

TEST(Scheduler, UpdateRuns) {
  auto world = ecs::world::world::create();
  world.insert_resource(counter_res{});

  auto sched = ecs::system::scheduler::create();
  sched.add_system(
      ecs::system::system_bind_point::update,
      [](ecs::resource::resource<counter_res> c) { c->value += 1; });

  sched.update(world);
  sched.update(world);
  sched.update(world);
  EXPECT_EQ(world.resource<counter_res>().value, 3);
}

TEST(Scheduler, ShutdownRuns) {
  auto world = ecs::world::world::create();
  world.insert_resource(counter_res{});

  auto sched = ecs::system::scheduler::create();
  sched.add_system(
      ecs::system::system_bind_point::shutdown,
      [](ecs::resource::resource<counter_res> c) { c->value = 99; });

  sched.shutdown(world);
  EXPECT_EQ(world.resource<counter_res>().value, 99);
}

TEST(Scheduler, BindPointsAreIndependent) {
  auto world = ecs::world::world::create();
  world.insert_resource(counter_res{});

  auto sched = ecs::system::scheduler::create();

  sched.add_system(
      ecs::system::system_bind_point::init,
      [](ecs::resource::resource<counter_res> c) { c->value += 1; });
  sched.add_system(
      ecs::system::system_bind_point::update,
      [](ecs::resource::resource<counter_res> c) { c->value += 10; });
  sched.add_system(
      ecs::system::system_bind_point::shutdown,
      [](ecs::resource::resource<counter_res> c) { c->value += 100; });

  sched.init(world);
  EXPECT_EQ(world.resource<counter_res>().value, 1);

  sched.update(world);
  EXPECT_EQ(world.resource<counter_res>().value, 11);

  sched.shutdown(world);
  EXPECT_EQ(world.resource<counter_res>().value, 111);
}

TEST(Scheduler, MultipleSystemsRunInOrder) {
  auto world = ecs::world::world::create();
  world.insert_resource(counter_res{});

  auto sched = ecs::system::scheduler::create();
  sched.add_system(
      ecs::system::system_bind_point::update,
      [](ecs::resource::resource<counter_res> c) {
        c->value = c->value * 2 + 1;
      });
  sched.add_system(
      ecs::system::system_bind_point::update,
      [](ecs::resource::resource<counter_res> c) {
        c->value = c->value * 2 + 1;
      });

  sched.update(world);
  // first: 0*2+1 = 1, second: 1*2+1 = 3
  EXPECT_EQ(world.resource<counter_res>().value, 3);
}

// ── resource param tests
// ──────────────────────────────────────────────────────

TEST(SystemParam, ResourceRead) {
  auto world = ecs::world::world::create();
  world.insert_resource(delta_res{.value = 0.5f});

  auto sched = ecs::system::scheduler::create();
  sched.add_system(
      ecs::system::system_bind_point::update,
      [](ecs::resource::resource_const<delta_res> dt,
          ecs::resource::resource<counter_res> c) {
        // should not compile — counter_res not inserted, will assert at runtime
      });

  // just test resource_const reads correctly
  world.insert_resource(counter_res{});
  sched.add_system(
      ecs::system::system_bind_point::init,
      [](ecs::resource::resource_const<delta_res> dt,
          ecs::resource::resource<counter_res> c) {
        c->value = static_cast<int>(dt->value * 100);
      });

  sched.init(world);
  EXPECT_EQ(world.resource<counter_res>().value, 50);
}

TEST(SystemParam, ResourceMutate) {
  auto world = ecs::world::world::create();
  world.insert_resource(counter_res{.value = 10});

  auto sched = ecs::system::scheduler::create();
  sched.add_system(
      ecs::system::system_bind_point::update,
      [](ecs::resource::resource<counter_res> c) { c->value *= 2; });

  sched.update(world);
  EXPECT_EQ(world.resource<counter_res>().value, 20);
}

// ── local param tests
// ─────────────────────────────────────────────────────────

TEST(SystemParam, LocalPersistsBetweenCalls) {
  auto world = ecs::world::world::create();
  world.insert_resource(counter_res{});

  auto sched = ecs::system::scheduler::create();
  sched.add_system(
      ecs::system::system_bind_point::update,
      [](ecs::resource::local<int> frame,
          ecs::resource::resource<counter_res> c) {
        ++(*frame);
        c->value = *frame;
      });

  sched.update(world);
  EXPECT_EQ(world.resource<counter_res>().value, 1);

  sched.update(world);
  EXPECT_EQ(world.resource<counter_res>().value, 2);

  sched.update(world);
  EXPECT_EQ(world.resource<counter_res>().value, 3);
}

TEST(SystemParam, MultipleLocalsIndependent) {
  auto world = ecs::world::world::create();
  world.insert_resource(counter_res{});

  auto sched = ecs::system::scheduler::create();

  // two systems each with their own Local<int> — should be independent
  sched.add_system(
      ecs::system::system_bind_point::update,
      [](ecs::resource::local<int> a) { ++(*a); });

  sched.add_system(
      ecs::system::system_bind_point::update,
      [](ecs::resource::local<int> b, ecs::resource::resource<counter_res> c) {
        ++(*b);
        c->value = *b;
      });

  sched.update(world);
  sched.update(world);
  // second system's local increments independently from first system's local
  EXPECT_EQ(world.resource<counter_res>().value, 2);
}

TEST(SystemParam, LocalInterleavedWithOtherParams) {
  auto world = ecs::world::world::create();
  world.insert_resource(counter_res{});
  world.insert_resource(delta_res{.value = 2.f});

  auto sched = ecs::system::scheduler::create();
  sched.add_system(
      ecs::system::system_bind_point::update,
      [](ecs::resource::local<float> accum,
          ecs::resource::resource_const<delta_res> dt,
          ecs::resource::local<int> ticks,
          ecs::resource::resource<counter_res> c) {
        *accum += dt->value;
        ++(*ticks);
        c->value = *ticks;
      });

  sched.update(world);
  sched.update(world);
  EXPECT_EQ(world.resource<counter_res>().value, 2);
}

// ── query param tests
// ─────────────────────────────────────────────────────────

TEST(SystemParam, QueryInSystem) {
  auto world = ecs::world::world::create();

  auto e1 = world.create_entity();
  auto e2 = world.create_entity();
  world.add_component(e1, position{.x = 0.f, .y = 0.f, .z = 0.f});
  world.add_component(e1, velocity{.dx = 1.f, .dy = 0.f, .dz = 0.f});
  world.add_component(e2, position{.x = 5.f, .y = 0.f, .z = 0.f});
  world.add_component(e2, velocity{.dx = 2.f, .dy = 0.f, .dz = 0.f});

  auto sched = ecs::system::scheduler::create();
  sched.add_system(
      ecs::system::system_bind_point::update,
      [](ecs::query::query<position, const velocity> q) {
        q.for_each([](position &p, const velocity &v) { p.x += v.dx; });
      });

  sched.update(world);
  EXPECT_FLOAT_EQ(world.get_component<position>(e1).x, 1.f);
  EXPECT_FLOAT_EQ(world.get_component<position>(e2).x, 7.f);
}

TEST(SystemParam, QueryWithResourceInSystem) {
  auto world = ecs::world::world::create();
  world.insert_resource(delta_res{.value = 2.f});

  auto e = world.create_entity();
  world.add_component(e, position{.x = 0.f, .y = 0.f, .z = 0.f});
  world.add_component(e, velocity{.dx = 1.f, .dy = 0.f, .dz = 0.f});

  auto sched = ecs::system::scheduler::create();
  sched.add_system(
      ecs::system::system_bind_point::update,
      [](ecs::query::query<position, const velocity> q,
          ecs::resource::resource_const<delta_res> dt) {
        q.for_each(
            [&dt](position &p, const velocity &v) { p.x += v.dx * dt->value; });
      });

  sched.update(world);
  EXPECT_FLOAT_EQ(world.get_component<position>(e).x, 2.f);
}

struct tag_camera {};
struct tag_enemy {};
struct tag_player {};

TEST(QueryFilter, WithOnlyMatchesEntitiesWithTag) {
  auto world = ecs::world::world::create();

  auto e1 = world.create_entity();
  auto e2 = world.create_entity();
  auto e3 = world.create_entity();
  world.add_component(e1, position{.x = 1.f, .y = 0.f, .z = 0.f});
  world.add_component(e1, tag_camera{});
  world.add_component(e2, position{.x = 2.f, .y = 0.f, .z = 0.f});
  world.add_component(e2, tag_camera{});
  world.add_component(e3, position{.x = 3.f, .y = 0.f, .z = 0.f});
  // e3 has no tag_camera

  world.insert_resource(counter_res{});
  auto sched = ecs::system::scheduler::create();
  sched.add_system(
      ecs::system::system_bind_point::update,
      [](ecs::query::query<position, ecs::query::with<tag_camera>> q,
         ecs::resource::resource<counter_res> c) {
        q.for_each([&c](position &) { c->value += 1; });
      });

  sched.update(world);
  // only e1 and e2 have tag_camera
  EXPECT_EQ(world.resource<counter_res>().value, 2);
}

TEST(QueryFilter, WithoutExcludesEntitiesWithTag) {
  auto world = ecs::world::world::create();

  auto e1 = world.create_entity();
  auto e2 = world.create_entity();
  auto e3 = world.create_entity();
  world.add_component(e1, position{.x = 1.f, .y = 0.f, .z = 0.f});
  world.add_component(e2, position{.x = 2.f, .y = 0.f, .z = 0.f});
  world.add_component(e2, tag_enemy{});
  world.add_component(e3, position{.x = 3.f, .y = 0.f, .z = 0.f});
  world.add_component(e3, tag_enemy{});

  world.insert_resource(counter_res{});
  auto sched = ecs::system::scheduler::create();
  sched.add_system(
      ecs::system::system_bind_point::update,
      [](ecs::query::query<position, ecs::query::without<tag_enemy>> q,
         ecs::resource::resource<counter_res> c) {
        q.for_each([&c](position &) { c->value += 1; });
      });

  sched.update(world);
  // only e1 has no tag_enemy
  EXPECT_EQ(world.resource<counter_res>().value, 1);
}

TEST(QueryFilter, WithAndWithoutCombined) {
  auto world = ecs::world::world::create();

  auto e1 = world.create_entity(); // player, no enemy — should match
  auto e2 = world.create_entity(); // player + enemy — should not match
  auto e3 = world.create_entity(); // no player — should not match
  world.add_component(e1, position{.x = 1.f, .y = 0.f, .z = 0.f});
  world.add_component(e1, tag_player{});
  world.add_component(e2, position{.x = 2.f, .y = 0.f, .z = 0.f});
  world.add_component(e2, tag_player{});
  world.add_component(e2, tag_enemy{});
  world.add_component(e3, position{.x = 3.f, .y = 0.f, .z = 0.f});

  world.insert_resource(counter_res{});
  auto sched = ecs::system::scheduler::create();

  sched.add_system(ecs::system::system_bind_point::update,
                   [](ecs::query::query<position, ecs::query::with<tag_player>,
                                        ecs::query::without<tag_enemy>>
                          q,
                      ecs::resource::resource<counter_res> c) {
                     q.for_each([&c](position &) { c->value += 1; });
                   });
  sched.update(world);
  // only e1 matches
  EXPECT_EQ(world.resource<counter_res>().value, 1);
}
