module;
#include "ecs.hpp"

export module ecs;

export namespace ecs::app {
    using ecs::app::app;
    using ecs::app::exit_resource;
}

export namespace ecs::world {
    using ecs::world::world;
}

export namespace ecs::entity {
    using ecs::entity::entity;
}

export namespace ecs::system {
    using ecs::system::scheduler;
    using ecs::system::system_bind_point;
    using ecs::system::system_context;
}

export namespace ecs::query {
    using ecs::query::query;
    using ecs::query::with;
    using ecs::query::without;
}

export namespace ecs::resource {
    using ecs::resource::resource;
    using ecs::resource::resource_const;
    using ecs::resource::local;
}

export namespace ecs::commands {
    using ecs::commands::commands;
}

export namespace ecs::events {
    using ecs::events::event;
    using ecs::events::event_reader;
    using ecs::events::event_writer;
    using ecs::events::event_registry;
}
