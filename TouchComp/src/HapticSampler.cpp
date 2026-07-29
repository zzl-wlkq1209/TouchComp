#include "HapticSampler.h"

#include "HDU/hduError.h"

HDCallbackCode HDCALLBACK HapticSamplerCallback(void* data)
{
    HapticSampler* sampler = static_cast<HapticSampler*>(data);
    std::lock_guard<std::mutex> lock(sampler->mutex_);

    hdBeginFrame(sampler->device_);
    hdSetDoublev(HD_CURRENT_FORCE, sampler->outputForce_);
    hdGetDoublev(HD_CURRENT_POSITION, sampler->state_.position);
    hdGetDoublev(HD_CURRENT_VELOCITY, sampler->state_.velocity);
    hdGetIntegerv(HD_CURRENT_BUTTONS, &sampler->state_.currentButton);
    hdGetIntegerv(HD_LAST_BUTTONS, &sampler->state_.lastButton);
    hdEndFrame(sampler->device_);

    if ((sampler->state_.currentButton & HD_DEVICE_BUTTON_2) != 0 &&
        (sampler->state_.lastButton & HD_DEVICE_BUTTON_2) == 0)
        sampler->button2Pressed_.store(true);

    if (HD_DEVICE_ERROR(sampler->error_ = hdGetError()) && hduIsSchedulerError(&sampler->error_))
        return HD_CALLBACK_DONE;

    return HD_CALLBACK_CONTINUE;
}

bool HapticSampler::initialize()
{
    device_ = hdInitDevice(HD_DEFAULT_DEVICE);
    if (HD_DEVICE_ERROR(error_ = hdGetError()))
    {
        hduPrintError(stderr, &error_, "Failed to initialize haptic device");
        return false;
    }
    if (hdCheckCalibration() != HD_CALIBRATION_OK)
    {
        fprintf(stderr, "Haptic device calibration is not ready.\n");
        hdDisableDevice(device_);
        device_ = HD_INVALID_HANDLE;
        return false;
    }

    hdEnable(HD_FORCE_OUTPUT);
    callback_ = hdScheduleAsynchronous(HapticSamplerCallback, this, HD_MAX_SCHEDULER_PRIORITY);
    hdStartScheduler();
    if (HD_DEVICE_ERROR(error_ = hdGetError()))
    {
        hduPrintError(stderr, &error_, "Failed to start haptic scheduler");
        hdUnschedule(callback_);
        hdDisableDevice(device_);
        device_ = HD_INVALID_HANDLE;
        return false;
    }

    initialized_ = true;
    return true;
}

void HapticSampler::shutdown()
{
    if (!initialized_)
        return;

    hdStopScheduler();
    hdUnschedule(callback_);
    hdDisableDevice(device_);
    initialized_ = false;
    device_ = HD_INVALID_HANDLE;
}

void HapticSampler::readState(HapticState& state) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    state = state_;
}

bool HapticSampler::consumeButton2Press()
{
    return button2Pressed_.exchange(false);
}
