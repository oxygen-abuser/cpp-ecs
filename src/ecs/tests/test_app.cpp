#include <gtest/gtest.h>
#include "ecs.hpp"

struct counter_res { int value = 0; };
struct my_event    { int data; };
struct position    { float x, y, z; };

// ── create ────────────────────────────────────────────────────────────────────

TEST(App, CreateHasExitResource) {
    auto a = ecs::app::app::create();
    EXPECT_TRUE(a.world().get_resource<ecs::app::exit_resource>().has_value());
}

TEST(App, CreateHasEventRegistry) {
    auto a = ecs::app::app::create();
    EXPECT_TRUE(a.world().get_resource<ecs::events::event_registry>().has_value());
}

// ── insert_resource ───────────────────────────────────────────────────────────

TEST(App, InsertResource) {
    auto a = ecs::app::app::create();
    a.insert_resource(counter_res{ .value = 42 });
    ASSERT_TRUE(a.world().get_resource<counter_res>().has_value());
    EXPECT_EQ(a.world().resource<counter_res>().value, 42);
}

TEST(App, InsertResourceChained) {
    auto a = ecs::app::app::create()
        .insert_resource(counter_res{ .value = 10 });
    EXPECT_EQ(a.world().resource<counter_res>().value, 10);
}

// ── add_system ────────────────────────────────────────────────────────────────

TEST(App, InitSystemRuns) {
    auto a = ecs::app::app::create();
    a.insert_resource(counter_res{});
    a.add_system(ecs::system::system_bind_point::init,
        [](ecs::resource::resource<counter_res> c) { c->value += 1; });

    a.init();
    EXPECT_EQ(a.world().resource<counter_res>().value, 1);
}

TEST(App, UpdateSystemRuns) {
    auto a = ecs::app::app::create();
    a.insert_resource(counter_res{});
    a.add_system(ecs::system::system_bind_point::update,
        [](ecs::resource::resource<counter_res> c) { c->value += 1; });

    a.init();
    a.tick();
    EXPECT_EQ(a.world().resource<counter_res>().value, 1);
    a.tick();
    EXPECT_EQ(a.world().resource<counter_res>().value, 2);
    a.shutdown();
}

TEST(App, ShutdownSystemRuns) {
    auto a = ecs::app::app::create();
    a.insert_resource(counter_res{});
    a.add_system(ecs::system::system_bind_point::shutdown,
        [](ecs::resource::resource<counter_res> c) { c->value = 99; });

    a.init();
    a.tick();
    a.shutdown();
    EXPECT_EQ(a.world().resource<counter_res>().value, 99);
}

TEST(App, MultipleSystemsSameBindPoint) {
    auto a = ecs::app::app::create();
    a.insert_resource(counter_res{});
    a.add_system(ecs::system::system_bind_point::update,
        [](ecs::resource::resource<counter_res> c) { c->value += 1; });
    a.add_system(ecs::system::system_bind_point::update,
        [](ecs::resource::resource<counter_res> c) { c->value += 10; });

    a.init();
    a.tick();
    EXPECT_EQ(a.world().resource<counter_res>().value, 11);
    a.shutdown();
}

// ── add_event ─────────────────────────────────────────────────────────────────

TEST(App, AddEventRegistersResource) {
    auto a = ecs::app::app::create();
    a.add_event<my_event>();
    EXPECT_TRUE(a.world().get_resource<ecs::events::event<my_event>>().has_value());
}

TEST(App, EventWriterAndReader) {
    auto a = ecs::app::app::create();
    a.add_event<my_event>();
    a.insert_resource(counter_res{});

    a.add_system(ecs::system::system_bind_point::update,
        [](ecs::events::event_writer<my_event> writer) {
            writer.send(my_event{ .data = 5 });
        });
    a.add_system(ecs::system::system_bind_point::update,
        [](ecs::events::event_reader<my_event> reader,
           ecs::resource::resource<counter_res> c) {
            for (auto& e : reader)
                c->value += e.data;
        });

    a.init();
    a.tick();  // frame 1: write, reader sees empty old buffer
    EXPECT_EQ(a.world().resource<counter_res>().value, 0);
    a.tick();  // frame 2: write, reader sees frame 1 event
    EXPECT_EQ(a.world().resource<counter_res>().value, 5);
    a.tick();  // frame 3: reader sees frame 2 event
    EXPECT_EQ(a.world().resource<counter_res>().value, 10);
    a.shutdown();
}

TEST(App, EventsClearAfterTwoFrames) {
    auto a = ecs::app::app::create();
    a.add_event<my_event>();
    a.insert_resource(counter_res{});

    // only send on first tick
    int tick_count = 0;
    a.add_system(ecs::system::system_bind_point::update,
        [&tick_count](ecs::events::event_writer<my_event> writer,
                      ecs::resource::resource<counter_res> c) {
            if (c->value == 0)
                writer.send(my_event{ .data = 1 });
            c->value++;
        });
    a.add_system(ecs::system::system_bind_point::update,
        [](ecs::events::event_reader<my_event> reader,
           ecs::resource::resource<counter_res> c) {
            // just count reads — don't modify counter here
            (void)c;
            for ([[maybe_unused]] auto& e : reader) {}
        });

    a.init();
    a.tick();  // write event, counter = 1
    a.tick();  // reader sees event
    a.tick();  // reader sees nothing — event cleared
    a.shutdown();
}

// ── quit / run ────────────────────────────────────────────────────────────────

TEST(App, RunExitsOnShouldExit) {
    auto a = ecs::app::app::create();
    a.insert_resource(counter_res{});

    a.add_system(ecs::system::system_bind_point::update,
        [](ecs::resource::resource<counter_res> c,
           ecs::resource::resource<ecs::app::exit_resource> exit) {
            c->value += 1;
            if (c->value >= 3)
                exit->should_exit = true;
        });

    a.run();
    EXPECT_GE(a.world().resource<counter_res>().value, 3);
}

// ── plugins ───────────────────────────────────────────────────────────────────

struct counter_plugin {
    int initial_value = 0;

    auto build(ecs::app::app& app) -> void {
        app.insert_resource(counter_res{ .value = initial_value })
           .add_system(ecs::system::system_bind_point::update,
               [](ecs::resource::resource<counter_res> c) { c->value += 1; });
    }
};

TEST(App, PluginInsertsResourceAndSystem) {
    auto a = ecs::app::app::create();
    a.add_plugin(counter_plugin{ .initial_value = 10 });

    ASSERT_TRUE(a.world().get_resource<counter_res>().has_value());
    EXPECT_EQ(a.world().resource<counter_res>().value, 10);

    a.init();
    a.tick();
    EXPECT_EQ(a.world().resource<counter_res>().value, 11);
    a.shutdown();
}

TEST(App, PluginChaining) {
    auto a = ecs::app::app::create()
        .add_plugin(counter_plugin{ .initial_value = 5 })
        .add_event<my_event>();

    EXPECT_EQ(a.world().resource<counter_res>().value, 5);
    EXPECT_TRUE(a.world().get_resource<ecs::events::event<my_event>>().has_value());

    a.init();
    a.tick();
    EXPECT_EQ(a.world().resource<counter_res>().value, 6);
    a.shutdown();
}

TEST(App, MultiplePlugins) {
    struct event_plugin {
        auto build(ecs::app::app& app) -> void {
            app.add_event<my_event>();
        }
    };

    auto a = ecs::app::app::create()
        .add_plugin(counter_plugin{ .initial_value = 0 })
        .add_plugin(event_plugin{});

    EXPECT_TRUE(a.world().get_resource<counter_res>().has_value());
    EXPECT_TRUE(a.world().get_resource<ecs::events::event<my_event>>().has_value());

    a.init();
    a.tick();
    EXPECT_EQ(a.world().resource<counter_res>().value, 1);
    a.shutdown();
}
