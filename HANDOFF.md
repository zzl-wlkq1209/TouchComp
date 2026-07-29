# TouchComp 交接文档

更新日期：2026-07-30

## 1. 当前状态

TouchComp 当前是一个仅连接 OpenHaptics 主手的球面几何采样程序，用于后续建立位置相关的重力与机构偏置补偿模型。

- 已移出正式构建范围：UR、RTDE、USB-CAN、电机、穿刺机构、M812x 力传感器、Eigen、Boost。
- 程序只读取主手三维位置和速度，并持续输出零力。
- 改造主手仅使用 `Button 2`：到达并稳定在目标点后，按下该键记录一个采样点。
- 采样界面为中文；Release x64 已在本机通过编译。

当前 GitHub 分支为 `codex/sphere-sampling`。本地尚保留一批旧整机代码文件，但它们未被跟踪、未上传，也未包含在 `TouchComp.vcxproj` 的构建清单中。

## 2. 关键几何约束

主手改造机构使末端方向经过固定控制点，因此采样空间按球面而不是普通三维体积处理。

```text
C = (2.5221, 128.7860, 25.9742) mm
r = 174.0 mm
```

其中 `C` 是已精确拟合的控制点，`r` 是当前程序使用的参考球面半径。目标点由俯仰角 `e` 与方位角 `a` 生成：

```text
p = C + r * [cos(e)cos(a), cos(e)sin(a), sin(e)]
```

当前默认网格为 5 x 5，共 25 点：

- 俯仰角：-20、-10、0、10、20 度
- 方位角：-30、-15、0、15、30 度
- 到位误差：不超过 2 mm
- 稳定判据：20 ms 刷新周期下连续 25 次到位，即至少 0.5 秒

这些参数位于 `TouchComp/src/SamplingWindow.cpp` 顶部常量区。若工作空间、球半径或到位阈值需要调整，应修改该处，并重新验证机械安全范围。

## 3. 程序结构

| 文件 | 职责 |
| --- | --- |
| `TouchComp/src/main.cpp` | 创建 Qt 应用与采样窗口。 |
| `TouchComp/header/HapticSampler.h` | OpenHaptics 主手状态与 Button 2 事件接口。 |
| `TouchComp/src/HapticSampler.cpp` | 高频回调：读取位置/速度/按键，保持零力输出。 |
| `TouchComp/header/SamplingWindow.h` | Qt 采样窗口接口与状态。 |
| `TouchComp/src/SamplingWindow.cpp` | 目标点生成、到位判定、中文 UI、CSV 记录。 |
| `TouchComp/TouchComp.vcxproj` | 唯一正式构建工程，仅列出上述三个源文件。 |

`HapticSampler` 使用原子标志记录 `Button 2` 的按下沿。窗口侧定时轮询该标志，因此按住按键不会反复写入数据。

## 4. 构建与运行

### 4.1 依赖

- Visual Studio 2019（C++ 桌面开发工作负载、`v142` 工具集）
- Qt 5.15.2 MSVC 2019 x64 与 Qt VS Tools
- OpenHaptics Developer 3.5.0 或兼容版本

在每台电脑上配置环境变量：

```powershell
setx QT_DIR "D:\Qt\5.15.2\msvc2019_64"
setx OPENHAPTICS_DIR "D:\Program Files\OpenHaptics\Developer\3.5.0"
```

修改环境变量后需要重新打开终端或 Visual Studio。`QT_DIR` 必须直接包含 `include`、`lib`、`bin`。工程通过 `QT_DIR` 定位 Qt，不再依赖开发电脑上的绝对 Qt 路径。

### 4.2 构建

打开 `TouchComp.sln`，选择 `Release | x64` 后生成。也可以在已加载环境变量的 PowerShell 中运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-release.ps1
```

本机 Release 输出位置为：

```text
x64\TouchCompRelease\TouchComp.exe
```

若 Visual Studio 显示工程“已卸载”，首先检查本机是否安装 Qt VS Tools、Qt 5.15.2 MSVC 2019 x64、`v142`，以及 `QT_DIR` 是否有效。

## 5. 现场采样流程

1. 确认原整机控制程序未占用同一台主手，且不连接/不启动 UR、CAN、电机、针或力传感器。
2. 启动程序；若提示设备不可用，先完成 OpenHaptics 标定并检查 USB 连接。
3. 观察界面的目标角度和目标误差，缓慢将主手移动到目标点。
4. 状态显示“已到位”后，继续保持约 0.5 秒。
5. 按主手 `Button 2` 一次；程序记录当前点并自动切换到下一目标点。
6. 完成 25 点后，建议抽取中心点、四角点和两个中间点再次重复采样，检查重复性。

窗口中的“上一目标点”“下一目标点”“记录当前点”是鼠标备选操作；主手侧唯一使用的输入是 `Button 2`。

## 6. 输出数据

采样文件由程序自动生成：

```text
data/geometric_samples.csv
```

列定义如下：

```text
timestamp,sample_id,target_elevation_deg,target_azimuth_deg,
x_mm,y_mm,z_mm,target_error_mm
```

当前 CSV 记录的是几何采样，尚未记录补偿力。不要将该文件解释为物理补偿实验结果。

## 7. 安全边界

- 当前版本在采样期间始终对主手输出零力；不要把它当作补偿控制版本。
- 未通过 OpenHaptics 标定时，程序应拒绝启动设备回调。
- 退出窗口时会停止主手 scheduler 并禁用设备。
- 不要在未知的机械限位附近扩大采样角度；先小范围验证再调整网格。
- 任何将来增加的力输出都必须有独立使能开关、限幅、低通滤波和故障时归零策略。

## 8. 下一阶段建议

1. 基于 `geometric_samples.csv` 用 MATLAB 检查球面半径误差、覆盖区域和重复性。
2. 增加“静态补偿力人工标注/测量”流程，形成单独的 `calibration_samples.csv`，包含 `Fx/Fy/Fz`。
3. 先在 C++ 中验证常量力与严格限幅，再实现最近邻补偿。
4. 再实现反距离加权插值、工作空间外归零与低通滤波。
5. 记录补偿开/关重复试验日志，并用 MATLAB 绘制误差、力场和重复性结果。

## 9. Git 说明

- 推送前执行 `git status --short`，不要误将本地旧模块、构建产物或采样结果提交。
- 当前分支：`codex/sphere-sampling`。
- 远程仓库：`https://github.com/zzl-wlkq1209/TouchComp.git`。
- 若在其他电脑克隆，确认已检出包含当前提交的分支，再打开解决方案。
