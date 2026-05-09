#include "ecs-commands.hpp"
#include "ecs-system-context.hpp"
#include "ecs.hpp"

// components
struct transform {
  float x, y, z;
};
struct velocity {
  float dx, dy, dz;
};
struct camera {};
struct body_rigid {
  float mass;
};

struct shape {};
struct foo {};
struct bar {};

auto test_bare_components() -> void {
  auto world = ecs::world::world::create();
  auto e = world.create_entity();
  world.emplace_component<transform>(e, 1.f, 2.f, 3.f);
  world.emplace_component<velocity>(e, 0.f, 1.f, 0.f);

  auto commands = ecs::commands::commands{};
  auto context = ecs::system::system_context{world, commands};

  // bare types, no entity
  auto q = ecs::query::query<transform, const velocity>::fetch(context);
  q.for_each([](transform &t, const velocity &v) { t.x += v.dx; });
}

auto test_with() -> void {
  auto world = ecs::world::world::create();
  auto e = world.create_entity();
  world.emplace_component<transform>(e, 1.f, 2.f, 3.f);
  world.emplace_component<camera>(e);
  auto commands = ecs::commands::commands{};
  auto context = ecs::system::system_context{world, commands};

  // only entities that also have camera
  auto q =
      ecs::query::query<transform, ecs::query::with<camera>>::fetch(context);
  q.for_each([](transform &t) { t.x = 0.f; });
}

auto test_without() -> void {
  auto world = ecs::world::world::create();
  auto e1 = world.create_entity();
  auto e2 = world.create_entity();
  world.emplace_component<transform>(e1, 1.f, 2.f, 3.f);
  world.emplace_component<transform>(e2, 4.f, 5.f, 6.f);
  world.emplace_component<body_rigid>(e2, 10.f);

  auto commands = ecs::commands::commands{};
  auto context = ecs::system::system_context{world, commands};

  // only entities WITHOUT body_rigid — should only hit e1
  auto q = ecs::query::query<transform, ecs::query::without<body_rigid>>::fetch(
      context);
  q.for_each([](transform &t) { t.x = 0.f; });
}

auto test_entity() -> void {
  auto world = ecs::world::world::create();
  auto e = world.create_entity();
  world.emplace_component<transform>(e, 1.f, 2.f, 3.f);

  auto commands = ecs::commands::commands{};
  auto context = ecs::system::system_context{world, commands};

  // entity exposed as first arg
  auto q = ecs::query::query<ecs::entity::entity, transform>::fetch(context);
  q.for_each([](ecs::entity::entity e, transform &t) {
    t.x = static_cast<float>(entt::to_integral(e.entt));
  });
}

auto test_all_combined() -> void {
  auto world = ecs::world::world::create();
  auto e1 = world.create_entity();
  auto e2 = world.create_entity();
  world.emplace_component<transform>(e1, 1.f, 2.f, 3.f);
  world.emplace_component<velocity>(e1, 0.f, 1.f, 0.f);
  world.emplace_component<camera>(e1);
  world.emplace_component<transform>(e2, 4.f, 5.f, 6.f);
  world.emplace_component<velocity>(e2, 1.f, 0.f, 0.f);
  world.emplace_component<body_rigid>(e2, 10.f);

  auto commands = ecs::commands::commands{};
  auto context = ecs::system::system_context{world, commands};

  // entity + bare + with + without + const
  auto q =
      ecs::query::query<ecs::entity::entity, transform, const velocity,
                        ecs::query::with<camera, foo>,
                        ecs::query::without<body_rigid, bar>>::fetch(context);

  q.for_each(
      [](ecs::entity::entity e, transform &t, const velocity &v) { t.x += v.dx; });
}
