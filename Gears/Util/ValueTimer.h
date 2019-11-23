#pragma once
#include "../ScriptUtils.h"
#include "MathExt.h"
#include "Timer.h"
#include <string>
#include <functional>

template <typename T>
class ValueTimer {
public:
    ValueTimer(std::string unit, std::function<void(const std::string&)> func, T limA, T limB, T tolerance)
        : mLimA(limA), mLimB(limB), mTolerance(tolerance), mTriggered(false)
        , mUnit(std::move(unit)), mTimer(0), mFunc(func) {
    }

    void Update(T newVal) {
        if (Math::Near(newVal, mLimA, mTolerance)) {
            mTimer.Reset();
            mTriggered = false;
        }

        bool triggeredNow = false;
        if (mLimA < mLimB) {
            if (newVal > mLimB && !mTriggered) {
                triggeredNow = true;
            }
        }
        else {
            if (newVal < mLimB && !mTriggered) {
                triggeredNow = true;
            }
        }

        if (triggeredNow) {
            mTriggered = true;
            auto millis = mTimer.Elapsed();
            mFunc(fmt::format("Timer: \n{} - {} {}: {}.{:03d}",
                mLimA, mLimB, mUnit, millis / 1000, millis % 1000));
        }
    }

    T mLimA;
    T mLimB;
    T mTolerance;
    bool mTriggered;
    std::string mUnit;
protected:
    Timer mTimer;
    std::function<void(const std::string&)> mFunc;
};
