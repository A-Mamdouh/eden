#pragma once

#include "DemoMeshes.hpp"

#include <Eden/Eden.hpp>

#include <memory>

namespace Demo {

std::unique_ptr<Eden::World::Scene> buildDemoScene(Eden::Engine &engine, const DemoMeshes &meshes,
                                                    Eden::Rendering::Model signModel);

} // namespace Demo
