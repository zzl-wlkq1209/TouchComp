#include "SamplingWindow.h"

#include <cmath>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
constexpr double kControlPoint[3] = { 2.5221, 128.7860, 25.9742 };
constexpr double kRadiusMm = 174.0;
constexpr double kToleranceMm = 2.0;
constexpr int kStableTicks = 25;
constexpr double kElevations[] = { -20.0, -10.0, 0.0, 10.0, 20.0 };
constexpr double kAzimuths[] = { -30.0, -15.0, 0.0, 15.0, 30.0 };
constexpr double kPi = 3.14159265358979323846;

double radians(double degrees) { return degrees * kPi / 180.0; }
}

SamplingWindow::SamplingWindow(QWidget* parent) : QWidget(parent)
{
    setWindowTitle("TouchComp - 球面采样");
    resize(660, 300);
    auto* layout = new QVBoxLayout(this);
    auto* title = new QLabel("几何采样 | 到位稳定后按主手 Button 2 记录", this);
    title->setStyleSheet("font-weight: bold; font-size: 16px;");
    layout->addWidget(title);
    layout->addWidget(new QLabel("采样阶段始终输出零力。固定控制点：C=(2.5221, 128.7860, 25.9742) mm。", this));
    targetLabel_ = new QLabel(this);
    currentLabel_ = new QLabel(this);
    statusLabel_ = new QLabel(this);
    statusLabel_->setStyleSheet("font-weight: bold; color: #b00020;");
    layout->addWidget(targetLabel_);
    layout->addWidget(currentLabel_);
    layout->addWidget(statusLabel_);

    auto* controls = new QHBoxLayout;
    auto* previous = new QPushButton("上一目标点", this);
    auto* next = new QPushButton("下一目标点", this);
    auto* record = new QPushButton("记录当前点", this);
    controls->addWidget(previous);
    controls->addWidget(next);
    controls->addWidget(record);
    layout->addLayout(controls);
    connect(previous, &QPushButton::clicked, this, [this] {
        azimuthIndex_ = (azimuthIndex_ + 4) % 5;
        if (azimuthIndex_ == 4) elevationIndex_ = (elevationIndex_ + 4) % 5;
        updateTargetDisplay();
    });
    connect(next, &QPushButton::clicked, this, [this] {
        azimuthIndex_ = (azimuthIndex_ + 1) % 5;
        if (azimuthIndex_ == 0) elevationIndex_ = (elevationIndex_ + 1) % 5;
        updateTargetDisplay();
    });
    connect(record, &QPushButton::clicked, this, &SamplingWindow::recordSample);

    updateTargetDisplay();
    deviceReady_ = sampler_.initialize();
    if (!deviceReady_)
    {
        currentLabel_->setText("设备不可用：请检查主手连接和 OpenHaptics 标定状态。");
        return;
    }
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &SamplingWindow::updateState);
    timer_->start(20);
}

SamplingWindow::~SamplingWindow()
{
    sampler_.shutdown();
}

void SamplingWindow::updateTargetDisplay()
{
    targetLabel_->setText(QString("目标点 %1/25：俯仰角 %2 度，方位角 %3 度，球面半径 %4 mm")
        .arg(elevationIndex_ * 5 + azimuthIndex_ + 1)
        .arg(kElevations[elevationIndex_], 0, 'f', 1)
        .arg(kAzimuths[azimuthIndex_], 0, 'f', 1)
        .arg(kRadiusMm, 0, 'f', 1));
    stableTicks_ = 0;
}

bool SamplingWindow::targetIsStable(const HapticState& state, double& errorMm) const
{
    const double elevation = radians(kElevations[elevationIndex_]);
    const double azimuth = radians(kAzimuths[azimuthIndex_]);
    const double target[3] = {
        kControlPoint[0] + kRadiusMm * std::cos(elevation) * std::cos(azimuth),
        kControlPoint[1] + kRadiusMm * std::cos(elevation) * std::sin(azimuth),
        kControlPoint[2] + kRadiusMm * std::sin(elevation)
    };
    const double dx = state.position[0] - target[0];
    const double dy = state.position[1] - target[1];
    const double dz = state.position[2] - target[2];
    errorMm = std::sqrt(dx * dx + dy * dy + dz * dz);
    return errorMm <= kToleranceMm;
}

void SamplingWindow::updateState()
{
    HapticState state;
    sampler_.readState(state);
    const double dx = state.position[0] - kControlPoint[0];
    const double dy = state.position[1] - kControlPoint[1];
    const double dz = state.position[2] - kControlPoint[2];
    const double radius = std::sqrt(dx * dx + dy * dy + dz * dz);
    double errorMm = 0.0;
    const bool withinTolerance = targetIsStable(state, errorMm);
    stableTicks_ = withinTolerance ? stableTicks_ + 1 : 0;
    currentLabel_->setText(QString("当前位置：x=%1，y=%2，z=%3 mm | 半径=%4 mm | 目标误差=%5 mm")
        .arg(state.position[0], 0, 'f', 2).arg(state.position[1], 0, 'f', 2).arg(state.position[2], 0, 'f', 2)
        .arg(radius, 0, 'f', 2).arg(errorMm, 0, 'f', 2));
    if (stableTicks_ >= kStableTicks)
    {
        statusLabel_->setStyleSheet("font-weight: bold; color: #15803d;");
        statusLabel_->setText("已到位：请按主手 Button 2 记录此点。");
    }
    else
    {
        statusLabel_->setStyleSheet("font-weight: bold; color: #b00020;");
        statusLabel_->setText(QString("请移动到目标点并保持稳定：%1/%2 个稳定周期。").arg(stableTicks_).arg(kStableTicks));
    }
    if (sampler_.consumeButton2Press()) recordSample();
}

void SamplingWindow::recordSample()
{
    if (!deviceReady_) return;
    HapticState state;
    sampler_.readState(state);
    double errorMm = 0.0;
    if (!targetIsStable(state, errorMm) || stableTicks_ < kStableTicks)
    {
        QMessageBox::warning(this, "目标点未就绪", "请移动到目标点并保持稳定后再记录。");
        return;
    }
    QDir().mkpath("data");
    QFile file("data/geometric_samples.csv");
    const bool writeHeader = !file.exists();
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        QMessageBox::critical(this, "写入失败", "无法写入 data/geometric_samples.csv。");
        return;
    }
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    if (writeHeader)
        stream << "timestamp,sample_id,target_elevation_deg,target_azimuth_deg,x_mm,y_mm,z_mm,target_error_mm\n";
    stream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << ',' << ++sampleCount_ << ','
           << kElevations[elevationIndex_] << ',' << kAzimuths[azimuthIndex_] << ','
           << state.position[0] << ',' << state.position[1] << ',' << state.position[2] << ',' << errorMm << '\n';
    azimuthIndex_ = (azimuthIndex_ + 1) % 5;
    if (azimuthIndex_ == 0) elevationIndex_ = (elevationIndex_ + 1) % 5;
    updateTargetDisplay();
}
