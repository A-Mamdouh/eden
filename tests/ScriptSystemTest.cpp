#include <Eden/Services/SceneService/Components.hpp>
#include <Eden/Services/SceneService/SceneService.hpp>
#include <Eden/Systems/InputSystem/InputSystem.hpp>
#include <Eden/Systems/ScriptSystem/ScriptBehaviour.hpp>
#include <Eden/Systems/ScriptSystem/ScriptComponent.hpp>
#include <Eden/Systems/ScriptSystem/ScriptSystem.hpp>

#include "EdenTestBase.hpp"

using namespace Eden::Services;
using namespace Eden::World;
using namespace Eden::Systems;
using namespace Eden::Input;
using namespace Eden::Scripting;

class ScriptSystemTest : public EdenTest::EdenTestBase {};

namespace {

class RecordingScript : public ScriptBehaviour {
public:
  void onStart(InputState &) override { ++startCount; }
  void onUpdate(double dt, InputState &) override {
    ++updateCount;
    lastDt = dt;
  }

  int startCount{0};
  int updateCount{0};
  double lastDt{0.0};
};

class MoveScript : public ScriptBehaviour {
public:
  void onUpdate(double dt, InputState &) override {
    entity().getComponent<Transform>().position.x += static_cast<float>(dt);
  }
};

} // namespace

TEST_F(ScriptSystemTest, OnStartIsCalledExactlyOnce) {
  SceneService sceneService;
  sceneService.init(eventService);

  auto scenePtr = std::make_unique<Scene>();
  auto entity = scenePtr->createEntity();
  auto script = std::make_unique<RecordingScript>();
  RecordingScript *scriptPtr = script.get();
  entity.addComponent<Components::ScriptComponent>(entity, *scenePtr, std::move(script));
  sceneService.loadScene(std::move(scenePtr));

  ScriptSystem scriptSystem{sceneService};
  scriptSystem.init(eventService);

  // ScriptSystem's inputState_ only becomes non-null once InputSystem
  // has published at least one Events::InputStateUpdatedEvent, same as
  // in Engine's real per-frame ordering (Input before Script).
  InputSystem inputSystem;
  inputSystem.init(eventService);
  inputSystem.update(0.0);

  scriptSystem.update(0.1);
  scriptSystem.update(0.1);
  scriptSystem.update(0.1);

  EXPECT_EQ(scriptPtr->startCount, 1);
  EXPECT_EQ(scriptPtr->updateCount, 3);
}

TEST_F(ScriptSystemTest, OnUpdateReceivesTheFrameDt) {
  SceneService sceneService;
  sceneService.init(eventService);

  auto scenePtr = std::make_unique<Scene>();
  auto entity = scenePtr->createEntity();
  auto script = std::make_unique<RecordingScript>();
  RecordingScript *scriptPtr = script.get();
  entity.addComponent<Components::ScriptComponent>(entity, *scenePtr, std::move(script));
  sceneService.loadScene(std::move(scenePtr));

  ScriptSystem scriptSystem{sceneService};
  scriptSystem.init(eventService);

  // ScriptSystem's inputState_ only becomes non-null once InputSystem
  // has published at least one Events::InputStateUpdatedEvent, same as
  // in Engine's real per-frame ordering (Input before Script).
  InputSystem inputSystem;
  inputSystem.init(eventService);
  inputSystem.update(0.0);
  scriptSystem.update(0.25);

  EXPECT_DOUBLE_EQ(scriptPtr->lastDt, 0.25);
}

TEST_F(ScriptSystemTest, EntityWithNullBehaviourIsSkipped) {
  SceneService sceneService;
  sceneService.init(eventService);

  auto scenePtr = std::make_unique<Scene>();
  auto entity = scenePtr->createEntity();
  entity.addComponent<Components::ScriptComponent>(entity, *scenePtr);
  sceneService.loadScene(std::move(scenePtr));

  ScriptSystem scriptSystem{sceneService};
  scriptSystem.init(eventService);

  // ScriptSystem's inputState_ only becomes non-null once InputSystem
  // has published at least one Events::InputStateUpdatedEvent, same as
  // in Engine's real per-frame ordering (Input before Script).
  InputSystem inputSystem;
  inputSystem.init(eventService);
  inputSystem.update(0.0);

  EXPECT_NO_THROW(scriptSystem.update(0.016));
}

TEST_F(ScriptSystemTest, ScriptCanMutateItsOwnComponents) {
  SceneService sceneService;
  sceneService.init(eventService);

  auto scenePtr = std::make_unique<Scene>();
  auto entity = scenePtr->createEntity();
  entity.addComponent<Transform>();
  entity.addComponent<Components::ScriptComponent>(entity, *scenePtr, std::make_unique<MoveScript>());
  sceneService.loadScene(std::move(scenePtr));

  ScriptSystem scriptSystem{sceneService};
  scriptSystem.init(eventService);

  // ScriptSystem's inputState_ only becomes non-null once InputSystem
  // has published at least one Events::InputStateUpdatedEvent, same as
  // in Engine's real per-frame ordering (Input before Script).
  InputSystem inputSystem;
  inputSystem.init(eventService);
  inputSystem.update(0.0);
  scriptSystem.update(2.0);

  EXPECT_FLOAT_EQ(entity.getComponent<Transform>().position.x, 2.0f);
}
