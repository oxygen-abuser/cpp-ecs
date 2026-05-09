#include <gtest/gtest.h>
#include "ecs.hpp"

// test resources
struct health   { int value; };
struct velocity { float x, y, z; };

// test components
struct position { float x, y, z; };
struct tag_player {};

// ── resource tests ────────────────────────────────────────────────────────────

TEST(WorldResource, InsertAndGet) {
    auto world = ecs::world::world::create();
    world.insert_resource(health{ .value = 100 });

    auto h = world.get_resource<health>();
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->get().value, 100);
}

TEST(WorldResource, GetConst) {
    auto world = ecs::world::world::create();
    world.insert_resource(health{ .value = 42 });

    auto h = world.get_resource_const<health>();
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h->get().value, 42);
}

TEST(WorldResource, GetMissing) {
    auto world = ecs::world::world::create();

    auto h = world.get_resource<health>();
    EXPECT_FALSE(h.has_value());
}

TEST(WorldResource, InsertDuplicateAsserts) {
    auto world = ecs::world::world::create();
    world.insert_resource(health{ .value = 100 });
    EXPECT_DEATH(world.insert_resource(health{ .value = 200 }), ".*");
}

TEST(WorldResource, Remove) {
    auto world = ecs::world::world::create();
    world.insert_resource(health{ .value = 100 });

    auto removed = world.remove_resource<health>();
    EXPECT_TRUE(removed);

    auto h = world.get_resource<health>();
    EXPECT_FALSE(h.has_value());
}

TEST(WorldResource, RemoveMissing) {
    auto world = ecs::world::world::create();
    auto removed = world.remove_resource<health>();
    EXPECT_FALSE(removed);
}

TEST(WorldResource, Mutate) {
    auto world = ecs::world::world::create();
    world.insert_resource(health{ .value = 100 });

    world.resource<health>().value = 200;

    EXPECT_EQ(world.resource<health>().value, 200);
}

TEST(WorldResource, MultipleResources) {
    auto world = ecs::world::world::create();
    world.insert_resource(health{ .value = 50 });
    world.insert_resource(velocity{ .x = 1.f, .y = 2.f, .z = 3.f });

    EXPECT_EQ(world.resource<health>().value, 50);
    EXPECT_FLOAT_EQ(world.resource<velocity>().x, 1.f);
}

// ── entity tests ──────────────────────────────────────────────────────────────

TEST(WorldEntity, CreateEntity) {
    auto world = ecs::world::world::create();
    auto e = world.create_entity();
    EXPECT_TRUE(world.is_valid(e));
}

TEST(WorldEntity, CreateMultiple) {
    auto world = ecs::world::world::create();
    auto e1 = world.create_entity();
    auto e2 = world.create_entity();
    EXPECT_NE(e1.entt, e2.entt);
}

// ── component tests ───────────────────────────────────────────────────────────

TEST(WorldComponent, AddAndGet) {
    auto world = ecs::world::world::create();
    auto e = world.create_entity();
    world.add_component(e, position{ .x = 1.f, .y = 2.f, .z = 3.f });

    auto& p = world.get_component<position>(e);
    EXPECT_FLOAT_EQ(p.x, 1.f);
    EXPECT_FLOAT_EQ(p.y, 2.f);
    EXPECT_FLOAT_EQ(p.z, 3.f);
}

TEST(WorldComponent, GetConst) {
    auto world = ecs::world::world::create();
    auto e = world.create_entity();
    world.add_component(e, position{ .x = 5.f, .y = 0.f, .z = 0.f });

    const auto& p = world.get_component_const<position>(e);
    EXPECT_FLOAT_EQ(p.x, 5.f);
}

TEST(WorldComponent, Mutate) {
    auto world = ecs::world::world::create();
    auto e = world.create_entity();
    world.add_component(e, position{ .x = 0.f, .y = 0.f, .z = 0.f });

    world.get_component<position>(e).x = 99.f;
    EXPECT_FLOAT_EQ(world.get_component<position>(e).x, 99.f);
}

TEST(WorldComponent, HasComponent) {
    auto world = ecs::world::world::create();
    auto e = world.create_entity();

    EXPECT_FALSE(world.has_component<position>(e));
    world.add_component(e, position{ .x = 0.f, .y = 0.f, .z = 0.f });
    EXPECT_TRUE(world.has_component<position>(e));
}

TEST(WorldComponent, MultipleComponents) {
    auto world = ecs::world::world::create();
    auto e = world.create_entity();
    world.add_component(e, position{ .x = 1.f, .y = 2.f, .z = 3.f });
    world.add_component(e, tag_player{});

    EXPECT_TRUE(world.has_component<position>(e));
    EXPECT_TRUE(world.has_component<tag_player>(e));
}

TEST(WorldComponent, MissingComponentAsserts) {
    auto world = ecs::world::world::create();
    auto e = world.create_entity();
    EXPECT_DEATH(world.get_component<position>(e), ".*");
}

TEST(WorldComponent, ComponentsIndependentPerEntity) {
    auto world = ecs::world::world::create();
    auto e1 = world.create_entity();
    auto e2 = world.create_entity();
    world.add_component(e1, position{ .x = 1.f, .y = 0.f, .z = 0.f });
    world.add_component(e2, position{ .x = 2.f, .y = 0.f, .z = 0.f });

    EXPECT_FLOAT_EQ(world.get_component<position>(e1).x, 1.f);
    EXPECT_FLOAT_EQ(world.get_component<position>(e2).x, 2.f);
}
