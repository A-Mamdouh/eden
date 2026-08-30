#pragma once
#include "Eden/Services/IService.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace Eden::Services {

/// Intended as a fixed-size worker-thread pool for offloading work off
/// the main loop. Not currently constructed by Engine, and not currently
/// functional even if constructed manually: nothing populates workers_
/// or invokes workerLoop() as a thread entry point, so submit()ted jobs
/// are queued but never executed, and wait() would spin forever.
class JobService : public IService {

public:
  /// Unit of work submitted via submit().
  using Job = std::function<void()>;
  std::string getName() override { return "Job Service"; }

  JobService() = default;
  /// Queues `job` for execution on the next available worker thread.
  /// @param job Callable to run; see class docs re: workers not yet
  ///        being spawned.
  void submit(Job job);
  /// Busy-waits until every submitted job's activeJobs_ count reaches
  /// zero.
  void wait();
  void stop() override;
  ~JobService();

protected:
  void onInit() override { running_ = true; }

private:
  /// Intended worker thread body: pops and runs jobs until running_ is
  /// cleared. Currently never spawned onto a std::thread.
  void workerLoop();
  std::vector<std::thread> workers_;
  std::queue<Job> jobQueue_;
  std::mutex queueMutex_;
  std::condition_variable cv_;
  std::atomic<bool> running_ = false;
  std::atomic<std::size_t> activeJobs_ = 0;
};

} // namespace Eden::Services
