#pragma once

#include "DemoMeshes.hpp"

#include <Eden/Eden.hpp>

#include <memory>

namespace Demo {

std::unique_ptr<Eden::Scene> buildDemoScene(Eden::Engine &engine, const DemoMeshes &meshes,
                                            Eden::Model signModel);

} // namespace Demo
