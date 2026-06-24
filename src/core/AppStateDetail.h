#pragma once

#include <atomic>

namespace cm {
namespace detail {

class BackgroundWorkGuard {
public:
    explicit BackgroundWorkGuard(std::atomic<bool>& inProgress) : inProgress_(inProgress) {}
    ~BackgroundWorkGuard() { inProgress_.store(false); }

    BackgroundWorkGuard(const BackgroundWorkGuard&) = delete;
    BackgroundWorkGuard& operator=(const BackgroundWorkGuard&) = delete;

private:
    std::atomic<bool>& inProgress_;
};

}  // namespace detail
}  // namespace cm
