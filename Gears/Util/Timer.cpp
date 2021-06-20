#include "Timer.h"

#ifndef NO_NATIVES
#include <inc/natives.h>
#endif

#include <chrono>

inline auto now() {
    using namespace std::chrono;
    auto tEpoch = steady_clock::now().time_since_epoch();
    return duration_cast<milliseconds>(tEpoch).count();
}

Timer::Timer(int64_t timeout) :
    mPeriod(timeout),
    mPreviousTime(now()) {
}

void Timer::Reset() {
    mPreviousTime = now();
}

void Timer::Reset(int64_t newTimeout) {
    mPeriod = newTimeout;
    mPreviousTime = now();
}

bool Timer::Expired() const {
    return now() > mPreviousTime + mPeriod;
}

int64_t Timer::Elapsed() const {
    return now() - mPreviousTime;
}

int64_t Timer::Period() const {
    return mPeriod;
}

#ifndef NO_NATIVES
inline auto gameNow() {
    return MISC::GET_GAME_TIMER();
}

GameTimer::GameTimer(int64_t timeout) :
    mPeriod(timeout),
    mPreviousTime(now()) {
}

void GameTimer::Reset() {
    mPreviousTime = now();
}

void GameTimer::Reset(int64_t newTimeout) {
    mPeriod = newTimeout;
    mPreviousTime = now();
}

bool GameTimer::Expired() const {
    return now() > mPreviousTime + mPeriod;
}

int64_t GameTimer::Elapsed() const {
    return now() - mPreviousTime;
}

int64_t GameTimer::Period() const {
    return mPeriod;
}
#endif
