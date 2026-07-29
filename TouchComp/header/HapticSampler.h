#pragma once

#include <atomic>
#include <mutex>

#include "HD/hd.h"
#include "HD/hdScheduler.h"
#include "HDU/hduVector.h"

struct HapticState
{
    HDdouble position[3] = { 0.0, 0.0, 0.0 };
    HDdouble velocity[3] = { 0.0, 0.0, 0.0 };
    HDint currentButton = 0;
    HDint lastButton = 0;
};

// The device adapter always outputs zero force during geometric sampling.
class HapticSampler
{
public:
    bool initialize();
    void shutdown();
    void readState(HapticState& state) const;
    bool consumeButton2Press();

private:
    friend HDCallbackCode HDCALLBACK HapticSamplerCallback(void* data);

    hduVector3Dd outputForce_ = { 0.0, 0.0, 0.0 };
    HapticState state_;
    HHD device_ = HD_INVALID_HANDLE;
    HDSchedulerHandle callback_ = HD_INVALID_HANDLE;
    HDErrorInfo error_;
    mutable std::mutex mutex_;
    std::atomic<bool> button2Pressed_{ false };
    bool initialized_ = false;
};
