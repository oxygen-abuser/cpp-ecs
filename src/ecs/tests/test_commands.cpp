#include <gtest/gtest.h>
#include "ecs-commands.hpp"
#include "ecs-system-context.hpp"
#include "ecs.hpp"

struct position { float x, y, z; };
struct velocity { float dx, dy, dz; };
struct health   { int value; };
struct tag_enemy {};

// ── spawn tests ───────────────────────────────────────────────────────────────

TEST(Commands, SpawnCreatesEntity) {
    auto world = ecs::world::world::create();
    auto sched = ecs::system::scheduler::create();

    sched.add_system(ecs::system::system_bind_point::update,
        [](ecs::commands::commands& cmd) {
            cmd.spawn(position{ .x = 1.f, .y = 2.f, .z = 3.f });
        });

    EXPECT_EQ(world.count<position>(), 0);

    sched.update(world);

    EXPECT_EQ(world.count<position>(), 1);
}

TEST(Commands, SpawnedEntityHasComponents) {
    auto world = ecs::world::world::create();
    auto sched = ecs::system::scheduler::create();

    sched.add_system(ecs::system::system_bind_point::update,
        [](ecs::commands::commands& cmd) {
            cmd.spawn(position{ .x = 1.f, .y = 2.f, .z = 3.f },
                      velocity{ .dx = 4.f, .dy = 5.f, .dz = 6.f });
        });

    sched.update(world);

    auto commands = ecs::commands::commands{};
    auto ctx = ecs::system::system_context{world, commands};

    int count = 0;
    auto q = ecs::query::query<position, velocity>::fetch(ctx);
    q.for_each([&](position& p, velocity& v) {
        ++count;
        EXPECT_FLOAT_EQ(p.x, 1.f);
        EXPECT_FLOAT_EQ(v.dx, 4.f);
    });
    EXPECT_EQ(count, 1);
}

TEST(Commands, SpawnMultiple) {
    auto world = ecs::world::world::create();
    auto sched = ecs::system::scheduler::create();

    sched.add_system(ecs::system::system_bind_point::update,
        [](ecs::commands::commands& cmd) {
            cmd.spawn(position{ .x = 1.f, .y = 0.f, .z = 0.f });
            cmd.spawn(position{ .x = 2.f, .y = 0.f, .z = 0.f });
            cmd.spawn(position{ .x = 3.f, .y = 0.f, .z = 0.f });
        });

    sched.update(world);

    EXPECT_EQ(world.count<position>(), 3);
}

TEST(Commands, SpawnNotAppliedDuringSystemRun) {
    auto world = ecs::world::world::create();
    auto sched = ecs::system::scheduler::create();

    sched.add_system(ecs::system::system_bind_point::update,
        [](ecs::commands::commands& cmd) {
            cmd.spawn(position{ .x = 0.f, .y = 0.f, .z = 0.f });
        });

    int count_during = 0;
    sched.add_system(ecs::system::system_bind_point::update,
        [&count_during](ecs::query::query<position> q) {
            q.for_each([&count_during](position&) { ++count_during; });
        });

    sched.update(world);

    EXPECT_EQ(count_during, 0);
    EXPECT_EQ(world.count<position>(), 1);
}

// ── despawn tests ─────────────────────────────────────────────────────────────

TEST(Commands, DespawnRemovesEntity) {
    auto world = ecs::world::world::create();
    auto e = world.create_entity();
    world.add_component(e, position{ .x = 0.f, .y = 0.f, .z = 0.f });

    EXPECT_EQ(world.count<position>(), 1);
    EXPECT_TRUE(world.is_valid(e));

    auto sched = ecs::system::scheduler::create();
    sched.add_system(ecs::system::system_bind_point::update,
        [e](ecs::commands::commands& cmd) {
            cmd.despawn(e);
        });

    sched.update(world);

    EXPECT_EQ(world.count<position>(), 0);
    EXPECT_FALSE(world.is_valid(e));
}

TEST(Commands, DespawnNotAppliedDuringSystemRun) {
    auto world = ecs::world::world::create();
    auto e = world.create_entity();
    world.add_component(e, position{ .x = 0.f, .y = 0.f, .z = 0.f });

    auto sched = ecs::system::scheduler::create();

    sched.add_system(ecs::system::system_bind_point::update,
        [e](ecs::commands::commands& cmd) {
            cmd.despawn(e);
        });

    int count_during = 0;
    sched.add_system(ecs::system::system_bind_point::update,
        [&count_during](ecs::query::query<position> q) {
            q.for_each([&count_during](position&) { ++count_during; });
        });

    sched.update(world);

    EXPECT_EQ(count_during, 1);
    EXPECT_EQ(world.count<position>(), 0);
    EXPECT_FALSE(world.is_valid(e));
}

// ── insert_resource tests ─────────────────────────────────────────────────────

TEST(Commands, InsertResource) {
    auto world = ecs::world::world::create();
    auto sched = ecs::system::scheduler::create();

    sched.add_system(ecs::system::system_bind_point::update,
        [](ecs::commands::commands& cmd) {
            cmd.insert_resource(health{ .value = 42 });
        });

    EXPECT_FALSE(world.get_resource<health>().has_value());

    sched.update(world);

    ASSERT_TRUE(world.get_resource<health>().has_value());
    EXPECT_EQ(world.resource<health>().value, 42);
}

TEST(Commands, InsertResourceNotVisibleDuringSystemRun) {
    auto world = ecs::world::world::create();
    auto sched = ecs::system::scheduler::create();

    sched.add_system(ecs::system::system_bind_point::update,
        [](ecs::commands::commands& cmd) {
            cmd.insert_resource(health{ .value = 99 });
        });

    bool visible_during = false;
    sched.add_system(ecs::system::system_bind_point::update,
        [&visible_during](ecs::query::query<position> q) {
            visible_during = q.size() > 0;
        });

    sched.update(world);

    EXPECT_FALSE(visible_during);
    EXPECT_TRUE(world.get_resource<health>().has_value());
}

// ── commands + other params ───────────────────────────────────────────────────

TEST(Commands, CommandsWithResource) {
    auto world = ecs::world::world::create();
    world.insert_resource(health{ .value = 3 });

    auto sched = ecs::system::scheduler::create();
    sched.add_system(ecs::system::system_bind_point::update,
        [](ecs::commands::commands& cmd,
           ecs::resource::resource_const<health> h) {
            for (int i = 0; i < h->value; ++i)
                cmd.spawn(position{ .x = static_cast<float>(i), .y = 0.f, .z = 0.f });
        });

    sched.update(world);

    EXPECT_EQ(world.count<position>(), 3);
}

TEST(Commands, CommandsWithQuery) {
    auto world = ecs::world::world::create();
    auto e1 = world.create_entity();
    auto e2 = world.create_entity();
    world.add_component(e1, position{ .x = 0.f, .y = 0.f, .z = 0.f });
    world.add_component(e1, tag_enemy{});
    world.add_component(e2, position{ .x = 0.f, .y = 0.f, .z = 0.f });
    world.add_component(e2, tag_enemy{});

    EXPECT_EQ(world.count<position>(), 2);

    auto sched = ecs::system::scheduler::create();
    sched.add_system(ecs::system::system_bind_point::update,
        [](ecs::commands::commands& cmd,
           ecs::query::query<ecs::entity::entity,
                             ecs::query::with<tag_enemy>> q) {
            q.for_each([&cmd](ecs::entity::entity e) {
                cmd.despawn(e);
            });
        });

    sched.update(world);

    EXPECT_EQ(world.count<position>(), 0);
    int total = 0;
    world.each([&total](ecs::entity::entity) { ++total; });
    EXPECT_EQ(total, 0);
}
