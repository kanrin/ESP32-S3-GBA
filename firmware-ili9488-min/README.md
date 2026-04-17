# ESP32-S3 Handheld MVP

这是基于 `ESP32-S3-DevKitC-1 + ILI9488 + TF卡` 的掌机最小工程，包含：

- 显示驱动：`src/drivers/display_ili9488.*`
- 按键驱动（去抖）：`src/drivers/input_keys.*`
- TF 存储驱动：`src/drivers/storage_sd.*`
- 音频双阶段：`src/drivers/audio_pwm.*` 与 `src/drivers/audio_i2s_pcm5102.*`
- 应用菜单：`src/app/rom_menu.*`
- GBC 适配层：`src/emulator/gbc_adapter.*`

## ROM 放置约定

- TF 卡中创建目录：`/roms`
- 支持扫描后缀：`.gb`、`.gbc`、`.zip`

## 交互说明

- `UP/DOWN`：切换 ROM
- `A`：加载并进入运行
- `B`：切换音频阶段（PWM / I2S）
- `START + SELECT`：返回菜单

## 构建命令

```bash
cd "/Users/yangjianbo/Projects/EPS32-S3-GBA/firmware-ili9488-min"
python3 -m platformio run
python3 -m platformio run -t upload --upload-port /dev/cu.usbmodemXXXX
python3 -m platformio device monitor -b 115200 --port /dev/cu.usbmodemXXXX
```

## 显示文字不可见时

- 使用 `USER_SETUP_LOADED=1` 时必须加 **`LOAD_GLCD=1`**，否则内置点阵字模不会编进固件，只会看到色块、没有字。
- 固件已开启 `setSwapBytes(true)`，并在 `platformio.ini` 中使用 `TFT_RGB_ORDER=TFT_RGB`。
- 若红绿蓝自检颜色明显不对，可把 `TFT_RGB_ORDER` 改回 `TFT_BGR` 再编译。
- 启动画面为**白底黑字**与**黄底黑字**，便于肉眼确认字模输出。
