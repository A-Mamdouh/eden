#include "Eden/Services/JobService/JobService.hpp"

namespace Eden {

void JobService::submit(Job job) {
    activeJobs_++;
    {
        std::lock_guard lock(queueMutex_);
        jobQueue_.push(std::move(job));
    }
    cv_.notify_one();
}

void JobService::workerLoop() {
    while (running_) {
      Job job;
      {
        std::unique_lock lock(queueMutex_);
        cv_.wait(lock, [&]{
          return !jobQueue_.empty() || !running_;
        });
        if(!running_) return;
        job = std::move(jobQueue_.front());
        jobQueue_.pop();
      }
      job();
      activeJobs_--;
    }
  }

  void JobService::stop()
  {
    wait();
  }

  JobService::~JobService()
  {
    // Finish all jobs first
    wait();
  }

  void JobService::wait()
  {
    while(activeJobs_ > 0)
    {
      std::this_thread::yield();
    }
  }
}