#include <gtest/gtest.h>
#include "ecs.hpp"
#include "ecs-schedulable.hpp"

struct counter_res { int value = 0; };
struct flag_res    { bool value = false; };
struct active_res  { bool value = true; };

// ── run_if tests ──────────────────────────────────────────────────────────────

TEST(RunIf, SystemSkippedWhenConditionFalse) {
    auto a = ecs::app::app::create();
    a.insert_resource(counter_res{});
    a.insert_resource(flag_res{ .value = false });

    a.add_system(ecs::system::system_bind_point::update,
        ecs::schedulable::systems(
            [](ecs::resource::resource<counter_res> c) { c->value += 1; }
        ).run_if(
            [](ecs::resource::resource_const<flag_res> f) -> bool {
                return f->value;
            }
        ));

    a.init();
    a.tick();
    EXPECT_EQ(a.world().resource<counter_res>().value, 0);
    a.shutdown();
}

TEST(RunIf, SystemRunsWhenConditionTrue) {
    auto a = ecs::app::app::create();
    a.insert_resource(counter_res{});
    a.insert_resource(flag_res{ .value = true });

    a.add_system(ecs::system::system_bind_point::update,
        ecs::schedulable::systems(
            [](ecs::resource::resource<counter_res> c) { c->value += 1; }
        ).run_if(
            [](ecs::resource::resource_const<flag_res> f) -> bool {
                return f->value;
            }
        ));

    a.init();
    a.tick();
    EXPECT_EQ(a.world().resource<counter_res>().value, 1);
    a.shutdown();
}

TEST(RunIf, ConditionCheckedEveryTick) {
    auto a = ecs::app::app::create();
    a.insert_resource(counter_res{});
    a.insert_resource(flag_res{ .value = false });

    a.add_system(ecs::system::system_bind_point::update,
        ecs::schedulable::systems(
            [](ecs::resource::resource<counter_res> c) { c->value += 1; }
        ).run_if(
            [](ecs::resource::resource_const<flag_res> f) -> bool {
                return f->value;
            }
        ));

    a.init();
    a.tick();
    EXPECT_EQ(a.world().resource<counter_res>().value, 0);

    // enable flag
    a.world().resource<flag_res>().value = true;
    a.tick();
    EXPECT_EQ(a.world().resource<counter_res>().value, 1);

    // disable flag
    a.world().resource<flag_res>().value = false;
    a.tick();
    EXPECT_EQ(a.world().resource<counter_res>().value, 1);

    a.shutdown();
}

TEST(RunIf, MultipleSystemsInGroupSkippedTogether) {
    auto a = ecs::app::app::create();
    a.insert_resource(counter_res{});
    a.insert_resource(flag_res{ .value = false });

    a.add_system(ecs::system::system_bind_point::update,
        ecs::schedulable::systems(
            [](ecs::resource::resource<counter_res> c) { c->value += 1; },
            [](ecs::resource::resource<counter_res> c) { c->value += 10; }
        ).run_if(
            [](ecs::resource::resource_const<flag_res> f) -> bool {
                return f->value;
            }
        ));

    a.init();
    a.tick();
    EXPECT_EQ(a.world().resource<counter_res>().value, 0);

    a.world().resource<flag_res>().value = true;
    a.tick();
    EXPECT_EQ(a.world().resource<counter_res>().value, 11);

    a.shutdown();
}

TEST(RunIf, ConditionAndComposition) {
    auto a = ecs::app::app::create();
    a.insert_resource(counter_res{});
    a.insert_resource(flag_res{ .value = true });
    a.insert_resource(active_res{ .value = true });

    // both conditions must be true
    a.add_system(ecs::system::system_bind_point::update,
        ecs::schedulable::systems(
            [](ecs::resource::resource<counter_res> c) { c->value += 1; }
        ).run_if(
            [](ecs::resource::resource_const<flag_res> f) -> bool {
                return f->value;
            }
        ).run_if(
            [](ecs::resource::resource_const<active_res> a) -> bool {
                return a->value;
            }
        ));

    a.init();
    a.tick();
    EXPECT_EQ(a.world().resource<counter_res>().value, 1);

    // disable one condition
    a.world().resource<flag_res>().value = false;
    a.tick();
    EXPECT_EQ(a.world().resource<counter_res>().value, 1);

    // re-enable but disable other
    a.world().resource<flag_res>().value = true;
    a.world().resource<active_res>().value = false;
    a.tick();
    EXPECT_EQ(a.world().resource<counter_res>().value, 1);

    a.shutdown();
}

TEST(RunIf, IndependentGroupsHaveIndependentConditions) {
    auto a = ecs::app::app::create();
    a.insert_resource(counter_res{});
    a.insert_resource(flag_res{ .value = false });
    a.insert_resource(active_res{ .value = true });

    // group 1 — gated by flag
    a.add_system(ecs::system::system_bind_point::update,
        ecs::schedulable::systems(
            [](ecs::resource::resource<counter_res> c) { c->value += 1; }
        ).run_if(
            [](ecs::resource::resource_const<flag_res> f) -> bool {
                return f->value;
            }
        ));

    // group 2 — gated by active
    a.add_system(ecs::system::system_bind_point::update,
        ecs::schedulable::systems(
            [](ecs::resource::resource<counter_res> c) { c->value += 10; }
        ).run_if(
            [](ecs::resource::resource_const<active_res> a) -> bool {
                return a->value;
            }
        ));

    a.init();
    a.tick();
    // flag=false, active=true → only group 2 runs
    EXPECT_EQ(a.world().resource<counter_res>().value, 10);

    a.world().resource<flag_res>().value = true;
    a.world().resource<active_res>().value = false;
    a.tick();
    // flag=true, active=false → only group 1 runs
    EXPECT_EQ(a.world().resource<counter_res>().value, 11);

    a.shutdown();
}

TEST(RunIf, NestedGroupConditionsAreAnded) {
    auto a = ecs::app::app::create();
    a.insert_resource(counter_res{});
    a.insert_resource(flag_res{ .value = true });
    a.insert_resource(active_res{ .value = true });

    auto inner = ecs::schedulable::systems(
        [](ecs::resource::resource<counter_res> c) { c->value += 1; }
    ).run_if(
        [](ecs::resource::resource_const<flag_res> f) -> bool {
            return f->value;
        }
    );

    // wrap inner with outer condition — ANDs with inner condition
    a.add_system(ecs::system::system_bind_point::update,
        ecs::schedulable::systems(std::move(inner))
        .run_if(
            [](ecs::resource::resource_const<active_res> a) -> bool {
                return a->value;
            }
        ));

    a.init();
    a.tick();
    EXPECT_EQ(a.world().resource<counter_res>().value, 1);

    // outer condition false → system skipped even though inner is true
    a.world().resource<active_res>().value = false;
    a.tick();
    EXPECT_EQ(a.world().resource<counter_res>().value, 1);

    a.shutdown();
}

// ── chain tests ───────────────────────────────────────────────────────────────

TEST(Chain, SystemsRunInOrder) {
    auto a = ecs::app::app::create();
    a.insert_resource(counter_res{});

    std::vector<int> order;

    a.add_system(ecs::system::system_bind_point::update,
        ecs::schedulable::systems(
            [&order](ecs::resource::resource<counter_res> c) {
                order.push_back(1);
                c->value = 1;
            },
            [&order](ecs::resource::resource<counter_res> c) {
                order.push_back(2);
                c->value = 2;
            },
            [&order](ecs::resource::resource<counter_res> c) {
                order.push_back(3);
                c->value = 3;
            }
        ).chain());

    a.init();
    a.tick();

    ASSERT_EQ(order.size(), 3);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
    a.shutdown();
}

TEST(Chain, ChainWithRunIf) {
    auto a = ecs::app::app::create();
    a.insert_resource(counter_res{});
    a.insert_resource(flag_res{ .value = true });

    std::vector<int> order;

    a.add_system(ecs::system::system_bind_point::update,
        ecs::schedulable::systems(
            [&order](ecs::resource::resource<counter_res> c) {
                order.push_back(1);
            },
            [&order](ecs::resource::resource<counter_res> c) {
                order.push_back(2);
            }
        ).chain()
         .run_if(
            [](ecs::resource::resource_const<flag_res> f) -> bool {
                return f->value;
            }
        ));

    a.init();
    a.tick();
    ASSERT_EQ(order.size(), 2);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);

    order.clear();
    a.world().resource<flag_res>().value = false;
    a.tick();
    EXPECT_TRUE(order.empty());

    a.shutdown();
}

TEST(Chain, TwoChainsAreIndependent) {
    auto a = ecs::app::app::create();
    a.insert_resource(counter_res{});

    std::vector<int> order;

    // chain 1
    a.add_system(ecs::system::system_bind_point::update,
        ecs::schedulable::systems(
            [&order](ecs::resource::resource<counter_res>) { order.push_back(1); },
            [&order](ecs::resource::resource<counter_res>) { order.push_back(2); }
        ).chain());

    // chain 2
    a.add_system(ecs::system::system_bind_point::update,
        ecs::schedulable::systems(
            [&order](ecs::resource::resource<counter_res>) { order.push_back(3); },
            [&order](ecs::resource::resource<counter_res>) { order.push_back(4); }
        ).chain());

    a.init();
    a.tick();

    ASSERT_EQ(order.size(), 4);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
    EXPECT_EQ(order[3], 4);
    a.shutdown();
}
