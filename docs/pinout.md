# ESP32-S3 掌机引脚冻结表（面包板版）

> 目标硬件：`ESP32-S3-DevKitC-1(N16R8)` + `ILI9488 SPI 480x320` + `TF卡` + `按键` + `PCM5102`

## 1. 显示屏（ILI9488 SPI）

| 功能 | ESP32-S3 GPIO | 说明 |
| --- | --- | --- |
| MOSI | GPIO11 | SPI 数据 |
| MISO | GPIO13 | 与屏 SDO 相连（FSPI 读回；与 TF 共用总线） |
| SCLK | GPIO12 | SPI 时钟 |
| CS | GPIO10 | 屏幕片选 |
| DC | GPIO9 | 数据/命令切换 |
| RST | GPIO14 | 硬复位 |
| BL | GPIO21 | 背光控制（可直连 3V3 常亮） |
| VCC/GND | 3V3/GND | 屏幕供电 |

## 2. TF 卡（SPI）

| 功能 | ESP32-S3 GPIO | 说明 |
| --- | --- | --- |
| CS | GPIO8 | TF 片选（已从 GPIO13 挪开，避免与 FSPI MISO 冲突） |
| MOSI | GPIO11 | 与屏幕复用 SPI 总线 |
| MISO | GPIO13 | 与屏幕复用 SPI 总线 |
| SCLK | GPIO12 | 与屏幕复用 SPI 总线 |
| VCC/GND | 3V3/GND | TF 模块供电 |

## 3. 按键（低电平触发，INPUT_PULLUP）

| 按键 | GPIO |
| --- | --- |
| UP | GPIO4 |
| DOWN | GPIO5 |
| LEFT | GPIO6 |
| RIGHT | GPIO7 |
| A | GPIO15 |
| B | GPIO16 |
| X | GPIO17 |
| Y | GPIO18 |
| START | GPIO47 |
| SELECT | GPIO48 |
| L | GPIO38 |
| R | GPIO39 |

## 4. 音频

### 阶段 1（PWM 快速验证）

| 功能 | GPIO |
| --- | --- |
| PWM OUT | GPIO42 |

### 阶段 2（PCM5102 I2S）

| 功能 | GPIO |
| --- | --- |
| BCLK | GPIO40 |
| LRCK/WS | GPIO41 |
| DOUT | GPIO42 |

## 5. 冻结规则

- 不再随意改动 SPI 与按键引脚，后续所有代码与焊接均按本表执行。
- 若必须改动，先改 `firmware-ili9488-min/src/boards/pin_config.h`，再更新本文档。
- 首次联调前先只接：屏幕 + 电源 + 串口，确认点屏后再逐步加入 TF/按键/音频。
