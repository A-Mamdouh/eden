#pragma once
#include "Eden/Services/IService.hpp"

#include "functional"
#include "queue"
#include "thread"
#include <condition_variable>
#include <mutex>

namespace Eden {

class JobService : public IService {

public:
  using Job = std::function<void()>;
  std::string getName() override { return "Job Service"; }

  JobService() = default;
  void submit(Job job);
  void wait();
  void stop() override;
  ~JobService();

protected:
  void onInit() override { running_ = true; }

private:
  void workerLoop();
  std::vector<std::thread> workers_;
  std::queue<Job> jobQueue_;
  std::mutex queueMutex_;
  std::condition_variable cv_;
  std::atomic<bool> running_ = false;
  std::atomic<std::size_t> activeJobs_ = 0;
};

} // namespace Eden