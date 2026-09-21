# Linux 机械革命蛟龙 16 Pro 2025 控制台
[![License](https://img.shields.io/badge/License-MIT-10b981.svg?style=flat-square)](LICENSE)
[![Framework](https://img.shields.io/badge/Framework-Slint%20+%20C++-f59e0b.svg?style=flat-square)](https://slint.dev/)
> 致力于让 蛟龙 16 Pro 2025 在 **Linux** 拥有完整体验
# 界面概览
<img width="700" alt="界面概览" src="https://github.com/user-attachments/assets/000f8463-c748-4461-a7dd-b384f2ed4960" />

# 亮点
- 使用 **Slint + C++** 开发, 启动迅速, 内存、硬盘占用小
- **完整实现**官方性能调节, 多模式自由切换
- 包含**硬件监控**功能, 可轻松监控硬件状态

# 兼容性
本工具目前在以下环境下完成验证：

| 组件 | 规格 / 版本 |
| :--- | :--- |
| **型号** | 机械革命 蛟龙 16 Pro 2025 |
| **CPU** | AMD Ryzen 9 9955HX |
| **显卡** | NVIDIA GeForce RTX 5070 Ti Laptop |
| **BIOS 版本** | `N.1.12MRO13` |
| **系统** | CachyOS + KDE |
---

# 快速上手
## 直接下载
1. 前往 [Releases](https://github.com/xjyzs/mechrevo-jiaolong-control-panel4linux/releases) 下载 .tar.gz 包
2. 解压并运行 ControlPanel
3. 按照提示安装 ACPI Call

## 从源码编译
1. Clone 本项目
2. 在项目根目录执行

``` shell
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
  -DCMAKE_INSTALL_RPATH='$ORIGIN' \
  -DCMAKE_CXX_FLAGS_RELEASE="-O3 -ffunction-sections -fdata-sections" \
  -DCMAKE_EXE_LINKER_FLAGS="-Wl,--gc-sections -Wl,--as-needed" \
  -DCMAKE_SHARED_LINKER_FLAGS="-Wl,--gc-sections -Wl,--as-needed"
cmake --build build --config Release
   ```

# 免责声明
本项目涉及对 EC 的直接读写操作。虽然已在指定机型上验证，但作者不对因误操作、固件不兼容等原因导致的硬件损坏或数据丢失承担责任。请在了解风险前提下使用。
