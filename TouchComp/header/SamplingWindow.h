#pragma once

#include <QtWidgets/QWidget>

#include "HapticSampler.h"

class QLabel;
class QTimer;

class SamplingWindow final : public QWidget
{
public:
    explicit SamplingWindow(QWidget* parent = nullptr);
    ~SamplingWindow() override;

private:
    void updateState();
    void recordSample();
    void updateTargetDisplay();
    bool targetIsStable(const HapticState& state, double& errorMm) const;

    HapticSampler sampler_;
    QTimer* timer_ = nullptr;
    QLabel* targetLabel_ = nullptr;
    QLabel* currentLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    int elevationIndex_ = 0;
    int azimuthIndex_ = 0;
    int stableTicks_ = 0;
    int sampleCount_ = 0;
    bool deviceReady_ = false;
};
