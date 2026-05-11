module;

#include <iostream>

module app;

import ecs;
import window;

auto run_app() -> void {
  ecs::app::app a = ecs::app::app::create();
  

  a.add_system(ecs::system::system_bind_point::init,
               []() { std::cout << "log\n"; });

  a.add_plugin(window::window_plugin::create({.title = "kek", .width = 1280, .height = 720}));
  a.run();
}
