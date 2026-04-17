# 装配与烧录说明（面包板阶段）

## 1. 最小装配顺序

1. 仅连接主控与屏幕：先验证点亮和刷屏。
2. 加入 TF 卡模块：验证 `/roms` 扫描与 ROM 读取。
3. 加入方向键和功能键：验证菜单交互。
4. 加入音频（先 PWM，后 I2S）：降低排错耦合。

## 2. 供电建议

- 调试阶段优先使用 USB 供电，避免电池与升压模块引入额外变量。
- 上电异常重启时优先检查：地线共地、线长、升压电流能力（建议 2A）。

## 3. 固件编译与烧录

```bash
cd "/Users/yangjianbo/Projects/EPS32-S3-GBA/firmware-ili9488-min"
python3 -m platformio run
python3 -m platformio device list
python3 -m platformio run -t upload --upload-port /dev/cu.usbmodemXXXX
python3 -m platformio device monitor -b 115200 --port /dev/cu.usbmodemXXXX
```

## 4. 常见问题

- 串口识别成蓝牙端口：手动指定 `--upload-port` 到 `usbmodem`/`SLAB_USBtoUART`。
- 屏幕背光亮但无图像：检查 `DC/CS/RST` 与 `TFT_eSPI` 引脚定义一致性。
- TF 初始化失败：检查 TF `CS`（现为 GPIO8）是否与 FSPI `MISO`（GPIO13）冲突；屏幕 `SDO` 须接 `GPIO13` 与 TF 共用 MISO。
