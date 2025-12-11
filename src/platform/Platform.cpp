#include "Eden/platform/Platform.hpp"
#include "Eden/core/Configuration.hpp"

namespace Eden {

std::unique_ptr<Platform> Platform::instance_ = nullptr;

Platform &Platform::getInstance() {
  if (instance_ == nullptr) {
    instance_ = std::unique_ptr<Platform>(new Platform());
  }
  return *instance_;
}

Platform::Platform() { input_ = std::make_shared<Input>(); }

void Platform::init(const ConfigurationService &config) {
  if (!input_) {
    input_ = std::make_shared<Input>();
  }
  window_ = createWindow(config.get().window);
}

void Platform::shutdown() {
  window_.reset();
  input_.reset();
}

std::shared_ptr<Window> Platform::getWindow() { return window_; }

std::shared_ptr<Input> Platform::getInput() { return input_; }

} // namespace Eden
