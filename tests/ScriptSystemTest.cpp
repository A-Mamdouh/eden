#include <Eden/Services/SceneService/Components.hpp>
#include <Eden/Services/SceneService/SceneService.hpp>
#include <Eden/Systems/InputSystem/InputSystem.hpp>
#include <Eden/Systems/ScriptSystem/ScriptBehaviour.hpp>
#include <Eden/Systems/ScriptSystem/ScriptComponent.hpp>
#include <Eden/Systems/ScriptSystem/ScriptSystem.hpp>

#include "EdenTestBase.hpp"

class ScriptSystemTest : public EdenTest::EdenTestBase {};

namespace {

class RecordingScript : public Eden::ScriptBehaviour {
public:
  void onStart(Eden::Entity, Eden::InputSystem &) override { ++startCount; }
  void onUpdate(Eden::Entity, double dt, Eden::InputSystem &) override {
    ++updateCount;
    lastDt = dt;
  }

  int startCount{0};
  int updateCount{0};
  double lastDt{0.0};
};

class MoveScript : public Eden::ScriptBehaviour {
public:
  void onUpdate(Eden::Entity entity, double dt, Eden::InputSystem &) override {
    entity.getComponent<Eden::Transform>().position.x += static_cast<float>(dt);
  }
};

} // namespace

TEST_F(ScriptSystemTest, OnStartIsCalledExactlyOnce) {
  Eden::SceneService sceneService;
  sceneService.init(eventService);

  auto scenePtr = std::make_unique<Eden::Scene>();
  auto entity = scenePtr->createEntity();
  auto script = std::make_unique<RecordingScript>();
  RecordingScript *scriptPtr = script.get();
  entity.addComponent<Eden::ScriptComponent>(Eden::ScriptComponent{.behaviour = std::move(script)});
  sceneService.loadScene(std::move(scenePtr));

  Eden::InputSystem inputSystem;
  Eden::ScriptSystem scriptSystem{sceneService, inputSystem};
  scriptSystem.init(eventService);

  scriptSystem.update(0.1);
  scriptSystem.update(0.1);
  scriptSystem.update(0.1);

  EXPECT_EQ(scriptPtr->startCount, 1);
  EXPECT_EQ(scriptPtr->updateCount, 3);
}

TEST_F(ScriptSystemTest, OnUpdateReceivesTheFrameDt) {
  Eden::SceneService sceneService;
  sceneService.init(eventService);

  auto scenePtr = std::make_unique<Eden::Scene>();
  auto entity = scenePtr->createEntity();
  auto script = std::make_unique<RecordingScript>();
  RecordingScript *scriptPtr = script.get();
  entity.addComponent<Eden::ScriptComponent>(Eden::ScriptComponent{.behaviour = std::move(script)});
  sceneService.loadScene(std::move(scenePtr));

  Eden::InputSystem inputSystem;
  Eden::ScriptSystem scriptSystem{sceneService, inputSystem};
  scriptSystem.init(eventService);
  scriptSystem.update(0.25);

  EXPECT_DOUBLE_EQ(scriptPtr->lastDt, 0.25);
}

TEST_F(ScriptSystemTest, EntityWithNullBehaviourIsSkipped) {
  Eden::SceneService sceneService;
  sceneService.init(eventService);

  auto scenePtr = std::make_unique<Eden::Scene>();
  auto entity = scenePtr->createEntity();
  entity.addComponent<Eden::ScriptComponent>();
  sceneService.loadScene(std::move(scenePtr));

  Eden::InputSystem inputSystem;
  Eden::ScriptSystem scriptSystem{sceneService, inputSystem};
  scriptSystem.init(eventService);

  EXPECT_NO_THROW(scriptSystem.update(0.016));
}

TEST_F(ScriptSystemTest, ScriptCanMutateItsOwnComponents) {
  Eden::SceneService sceneService;
  sceneService.init(eventService);

  auto scenePtr = std::make_unique<Eden::Scene>();
  auto entity = scenePtr->createEntity();
  entity.addComponent<Eden::Transform>();
  entity.addComponent<Eden::ScriptComponent>(Eden::ScriptComponent{.behaviour = std::make_unique<MoveScript>()});
  sceneService.loadScene(std::move(scenePtr));

  Eden::InputSystem inputSystem;
  Eden::ScriptSystem scriptSystem{sceneService, inputSystem};
  scriptSystem.init(eventService);
  scriptSystem.update(2.0);

  EXPECT_FLOAT_EQ(entity.getComponent<Eden::Transform>().position.x, 2.0f);
}
