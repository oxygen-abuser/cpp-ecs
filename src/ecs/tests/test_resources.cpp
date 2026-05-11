#include <gtest/gtest.h>
#include "ecs.hpp"

struct physics_res { float gravity = -9.81f; };
struct render_res  { int width = 1920; };
struct counter_res { int value = 0; };

// ── if_res skips system when resource missing ─────────────────────────────────

TEST(IfRes, SystemSkippedWhenResourceMissing) {
    auto world = ecs::world::world::create();
    world.insert_resource(counter_res{});

    auto sched = ecs::system::scheduler::create();
    sched.add_system(ecs::system::system_bind_point::update,
        [](ecs::resource::if_res<ecs::resource::resource<physics_res>> physics,
           ecs::resource::resource<counter_res> c) {
            c->value += 1;
        });

    sched.update(world);
    EXPECT_EQ(world.resource<counter_res>().value, 0);
}

TEST(IfRes, SystemRunsWhenResourceExists) {
    auto world = ecs::world::world::create();
    world.insert_resource(counter_res{});
    world.insert_resource(physics_res{});

    auto sched = ecs::system::scheduler::create();
    sched.add_system(ecs::system::system_bind_point::update,
        [](ecs::resource::if_res<ecs::resource::resource<physics_res>> physics,
           ecs::resource::resource<counter_res> c) {
            c->value += 1;
        });

    sched.update(world);
    EXPECT_EQ(world.resource<counter_res>().value, 1);
}

TEST(IfRes, AccessValueWhenResourceExists) {
    auto world = ecs::world::world::create();
    world.insert_resource(physics_res{ .gravity = -9.81f });
    world.insert_resource(counter_res{});

    auto sched = ecs::system::scheduler::create();
    sched.add_system(ecs::system::system_bind_point::update,
        [](ecs::resource::if_res<ecs::resource::resource<physics_res>> physics,
           ecs::resource::resource<counter_res> c) {
            c->value = static_cast<int>(physics->gravity * -1.f);
        });

    sched.update(world);
    EXPECT_EQ(world.resource<counter_res>().value, 9);
}

TEST(IfRes, SystemCheckedEveryTick) {
    auto world = ecs::world::world::create();
    world.insert_resource(counter_res{});

    auto sched = ecs::system::scheduler::create();
    sched.add_system(ecs::system::system_bind_point::update,
        [](ecs::resource::if_res<ecs::resource::resource<physics_res>> physics,
           ecs::resource::resource<counter_res> c) {
            c->value += 1;
        });

    // no physics — skipped
    sched.update(world);
    EXPECT_EQ(world.resource<counter_res>().value, 0);

    // insert physics — runs
    world.insert_resource(physics_res{});
    sched.update(world);
    EXPECT_EQ(world.resource<counter_res>().value, 1);

    // remove physics — skipped again
    world.remove_resource<physics_res>();
    sched.update(world);
    EXPECT_EQ(world.resource<counter_res>().value, 1);
}

TEST(IfRes, MultipleIfResAllMustExist) {
    auto world = ecs::world::world::create();
    world.insert_resource(counter_res{});
    world.insert_resource(physics_res{});
    // render_res missing

    auto sched = ecs::system::scheduler::create();
    sched.add_system(ecs::system::system_bind_point::update,
        [](ecs::resource::if_res<ecs::resource::resource<physics_res>> physics,
           ecs::resource::if_res<ecs::resource::resource<render_res>> render,
           ecs::resource::resource<counter_res> c) {
            c->value += 1;
        });

    sched.update(world);
    // render_res missing — system skipped
    EXPECT_EQ(world.resource<counter_res>().value, 0);

    world.insert_resource(render_res{});
    sched.update(world);
    // both exist — system runs
    EXPECT_EQ(world.resource<counter_res>().value, 1);
}

TEST(IfRes, WorksWithConstResource) {
    auto world = ecs::world::world::create();
    world.insert_resource(physics_res{ .gravity = -9.81f });
    world.insert_resource(counter_res{});

    auto sched = ecs::system::scheduler::create();
    sched.add_system(ecs::system::system_bind_point::update,
        [](ecs::resource::if_res<ecs::resource::resource_const<physics_res>> physics,
           ecs::resource::resource<counter_res> c) {
            c->value = static_cast<int>(physics->gravity * -1.f);
        });

    sched.update(world);
    EXPECT_EQ(world.resource<counter_res>().value, 9);
}

TEST(IfRes, WorksWithRunIf) {
    auto world = ecs::world::world::create();
    world.insert_resource(counter_res{});

    auto sched = ecs::system::scheduler::create();
    sched.add_system(ecs::system::system_bind_point::update,
        ecs::schedulable::systems(
            [](ecs::resource::if_res<ecs::resource::resource<physics_res>> physics,
               ecs::resource::resource<counter_res> c) {
                c->value += 1;
            }
        ).run_if([](ecs::resource::resource_const<counter_res> c) -> bool {
            return c->value < 5;
        }));

    sched.update(world);
    EXPECT_EQ(world.resource<counter_res>().value, 0);

    world.insert_resource(physics_res{});
    sched.update(world);
    EXPECT_EQ(world.resource<counter_res>().value, 1);
}
