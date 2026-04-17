# ESP32-S3 掌机回归与验收清单

## 0. 烧录流程（Cursor 终端）

```bash
cd "/Users/yangjianbo/Projects/EPS32-S3-GBA/firmware-ili9488-min"
python3 -m platformio device list
python3 -m platformio run
python3 -m platformio run -t upload --upload-port /dev/cu.usbmodemXXXX
python3 -m platformio device monitor -b 115200 --port /dev/cu.usbmodemXXXX
```

## 1. 基础硬件冒烟

- [ ] 上电后屏幕背光亮起。
- [ ] 串口输出出现 `Booting handheld MVP...`。
- [ ] 菜单界面显示 `ESP32-S3 GBC MVP`。
- [ ] 按 `UP/DOWN` 可切换 ROM 选择文本。

## 2. 存储与 ROM

- [ ] TF 卡可挂载（无 `SD init failed`）。
- [ ] `/roms` 目录可扫描到 `.gb/.gbc/.zip`。
- [ ] 按 `A` 后可进入运行状态（显示动态画面）。

## 3. 输入与交互

- [ ] A 键触发音效（PWM 阶段）。
- [ ] `START + SELECT` 可以回到菜单。
- [ ] 长时间按键无明显抖动误触。

## 4. 音频两阶段验收

### 阶段1：PWM
- [ ] 具备基础蜂鸣提示音。
- [ ] 运行 10 分钟无崩溃。

### 阶段2：I2S + PCM5102
- [ ] I2S 初始化成功（无错误日志）。
- [ ] 左右声道均有输出。
- [ ] 无明显底噪或断裂爆音。

## 5. 稳定性与续航

- [ ] 连续运行 15 分钟不重启。
- [ ] 屏幕、主控、升压模块温升可接受。
- [ ] 电池供电切换/充电状态下运行稳定。

## 6. 装配与复刻资料

- [ ] 引脚表与实物接线一致（参考 `docs/pinout.md`）。
- [ ] 记录固件版本和上传命令。
- [ ] 保留一份最小可运行 ROM 清单用于回归。
