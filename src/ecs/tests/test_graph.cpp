#include "ecs-system-traits.hpp"
#include "ecs.hpp"
#include <gtest/gtest.h>

struct counter_res {
  int value = 0;
};
struct shared_res {
  int value = 0;
};

TEST(Graph, DependentSystemsRunInOrder) {
  auto a = ecs::app::app::create();
  a.insert_resource(counter_res{});
  a.insert_resource(shared_res{});

  std::vector<int> order;

  // system A writes shared_res
  a.add_system(ecs::system::system_bind_point::update,
               [&order](ecs::resource::resource<shared_res> s) {
                 s->value = 1;
                 order.push_back(1);
               });

  // system B reads shared_res — should run after A
  a.add_system(ecs::system::system_bind_point::update,
               [&order](ecs::resource::resource_const<shared_res> s,
                        ecs::resource::resource<counter_res> c) {
                 c->value = s->value;
                 order.push_back(2);
               });

  a.init();
  a.tick();

  // B must run after A
  ASSERT_EQ(order.size(), 2);
  EXPECT_EQ(order[0], 1);
  EXPECT_EQ(order[1], 2);
  // B saw A's write
  EXPECT_EQ(a.world().resource<counter_res>().value, 1);
  a.shutdown();
}

TEST(Graph, IndependentSystemsCanRunInAnyOrder) {
  auto a = ecs::app::app::create();
  a.insert_resource(counter_res{});

  // two systems with no shared resources — no dependency between them
  a.add_system(ecs::system::system_bind_point::update,
               [](ecs::resource::resource<counter_res> c) { c->value += 1; });

  a.add_system(ecs::system::system_bind_point::update,
               [](ecs::resource::resource<counter_res> c) { c->value += 10; });

  a.init();
  a.tick();

  // both ran — order doesn't matter, result is deterministic
  EXPECT_EQ(a.world().resource<counter_res>().value, 11);
  a.shutdown();
}

TEST(Graph, ConflictingSystemsRunSequentially) {
  auto a = ecs::app::app::create();
  a.insert_resource(counter_res{});

  std::vector<int> writes;

  a.add_system(ecs::system::system_bind_point::update,
               [&writes](ecs::resource::resource<counter_res> c) {
                 c->value += 1;
                 writes.push_back(c->value);
               });

  a.add_system(ecs::system::system_bind_point::update,
               [&writes](ecs::resource::resource<counter_res> c) {
                 c->value += 10;
                 writes.push_back(c->value);
               });

  a.init();
  a.tick();

  // both ran, write-write conflict means sequential
  ASSERT_EQ(writes.size(), 2);
  ASSERT_EQ(writes[0], 1);
  ASSERT_EQ(writes[1], 11);
  EXPECT_EQ(a.world().resource<counter_res>().value, 11);
  a.shutdown();
}

TEST(Graph, ChainedSystemsRespectOrder) {
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
                     c->value += 10;
                   })
                   .chain());

  a.init();
  a.tick();

  ASSERT_EQ(order.size(), 2);
  EXPECT_EQ(order[0], 1);
  EXPECT_EQ(order[1], 2);
  EXPECT_EQ(a.world().resource<counter_res>().value, 11);
  a.shutdown();
}
