# dde-dock-privacy

Deepin 任务栏隐私指示器插件，用于显示当前正在被使用的隐私敏感设备。

## 功能

当以下设备被占用时，在任务栏显示对应的图标提示：

| 图标 | 设备 | 检测方式 |
|:---:|:---|:---|
| 📹 | 摄像头 | 检测 `/dev/video*` 设备节点是否被打开 |
| 🎤 | 麦克风 | 检测 `/proc/asound` 录音设备状态 |
| 📍 | 位置服务 | 检测 geoclue 服务状态 |

## 任务栏位置

插件显示在任务栏**左侧区域的最右边**：

```
┌─────────────────────────────────────────────────────────────────┐
│  [启动器] [通知中心] [隐私指示器] │ [任务图标区] │ [托盘] [时间]  │
│                                  │              │               │
│   dockOrder: 1-10               │   10-20      │    20-30      │
│                                  │              │               │
│  ◀─── 左侧区域 ───▶ │ ◀── 中间区域 ──▶ │ ◀── 右侧区域 ──▶  │
│                                  │              │               │
└─────────────────────────────────────────────────────────────────┘
                              ↑
                           你的插件在这里 (dockOrder: 10)
```

## 构建安装

```bash
mkdir build && cd build
cmake ..
make -j4
sudo make install
```

安装内容：
- `/usr/lib/x86_64-linux-gnu/dde-shell/org.deepin.ds.dock.privacyindicator.so`
- `/usr/share/dde-shell/org.deepin.ds.dock.privacyindicator/`

## 调试

```bash
# 强制显示插件（红色背景，用于调试）
# 修改 package/privacyindicator.qml 中的 debugMode: true

# 查看日志
DSG_LOG_LEVEL=debug dde-shell -p org.deepin.ds.dock 2>&1 | grep -i privacy
```

## 打包

### 本地构建 deb 包

```bash
sudo apt-get install build-essential debhelper cmake pkg-config
sudo apt-get build-dep .
dpkg-buildpackage -us -uc -b
```

### 使用 GitHub Actions

项目配置了自动构建：
- **Build**: 每次 push/PR 时编译检查
- **Debian Package**: 打 tag 时自动构建 deb 包并发布

## 许可证

GPL-3.0-or-later
