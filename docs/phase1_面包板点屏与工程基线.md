# 第一阶段总结：面包板点屏与工程基线（ESP32-S3 + ILI9488）

**阶段目标**：在面包板上用 ESP32-S3 驱动 3.5 寸 ILI9488（480×320，SPI），固件可稳定烧录、屏幕可显示色条与文字，并搭好后续掌机开发的工程骨架。

**文档日期**：2026-04

---

## 1. 环境与工具链

- **开发环境**：Cursor + PlatformIO；终端推荐用 `python3 -m platformio` 调用 Core（与 Cursor 插件解耦）。
- **曾遇到问题与处理**：
  - `pio` 不在 `PATH`：在 `~/.zshrc` 中加入 Python 用户脚本目录（例如 `~/Library/Python/3.9/bin`），**不要**使用无效的 `3.*` 通配写法。
  - Cursor 中 PlatformIO 插件长期 “Initializing”：可仅用终端完成 `run` / `upload` / `monitor`；或在用户设置中配置 `platformio-ide.customPATH`、关闭内置 Core 等（属 IDE 体验问题，不阻塞编译烧录）。
- **冒烟验证**：在临时目录创建最小 `platformio` 工程，确认 `pio run` 对 `esp32-s3-devkitc-1` 可成功编译。

---

## 2. 硬件与接线（本阶段冻结）

- **主控**：ESP32-S3（BOM 目标为 **ESP32-S3-DevKitC-1 N16R8**；PlatformIO 板型为 `esp32-s3-devkitc-1`）。
- **屏幕**：3.5 寸 **ILI9488**，**480×320**，**4-wire SPI**。
- **背光**：可先 **LED 接 3V3 常亮**（不占用 GPIO）；固件中可将 `pin_config.h` 里 `kBacklightOnGpio` 设为 `false`，避免误用 GPIO21。
- **显示 SPI 接线**（与 [pinout.md](pinout.md) 一致）：

| 屏引脚 | ESP32-S3 GPIO |
|--------|----------------|
| VCC | 3V3 |
| GND | GND |
| CS | GPIO10 |
| RESET | GPIO14 |
| DC | GPIO9 |
| SDI (MOSI) | GPIO11 |
| SCK | GPIO12 |
| SDO (MISO) | **GPIO13**（必读回，勿与 MOSI 混为同一脚） |
| LED | 3V3 或 GPIO21（按实物选择） |

- **TF 卡（下一阶段重点）**：与屏 **共用 MOSI/SCLK/MISO**；**片选 CS = GPIO8**（已从 GPIO13 挪开，避免与 FSPI MISO 冲突）。卡上目录约定：`/roms`，ROM 后缀 `.gb` / `.gbc` / `.zip`。

---

## 3. 固件工程位置与结构

| 路径 | 说明 |
|------|------|
| [firmware-ili9488-min/platformio.ini](../firmware-ili9488-min/platformio.ini) | 板型、`TFT_eSPI` 宏（引脚、驱动、`LOAD_GLCD`、`USE_HSPI_PORT`、SPI 频率、色序等） |
| [firmware-ili9488-min/src/main.cpp](../firmware-ili9488-min/src/main.cpp) | 启动流程、菜单、SD、模拟器入口 |
| [firmware-ili9488-min/src/boards/pin_config.h](../firmware-ili9488-min/src/boards/pin_config.h) | 与实物一致的 GPIO 常量 |
| [firmware-ili9488-min/src/drivers/display_ili9488.*](../firmware-ili9488-min/src/drivers/display_ili9488.cpp) | 屏初始化、自检色条、启动画面、文本绘制 |
| 其他 drivers / app / emulator | 按键、TF、音频、ROM 菜单、GBC 适配层（骨架） |

**配套文档**：[pinout.md](pinout.md)、[assembly_and_flash.md](assembly_and_flash.md)、[test_checklist.md](test_checklist.md)、[firmware-ili9488-min/README.md](../firmware-ili9488-min/README.md)。

---

## 4. 关键问题与对策（建议重点记录）

| 现象 | 原因 | 处理 |
|------|------|------|
| 上电 `Guru Meditation`，栈在 `TFT_eSPI` / `SPI.beginTransaction` | S3 上 SPI 未正确建立；未指定 MISO 时库可能把 MISO 与 MOSI 绑在同一脚 | 在 `platformio.ini` 增加 **`TFT_MISO=13`**；必要时启用 **`USE_HSPI_PORT`** |
| 只有色块、无文字 | 使用 `USER_SETUP_LOADED=1` 时未包含默认 `User_Setup.h`，若未定义 **`LOAD_GLCD`**，字模为空 | 增加 **`-D LOAD_GLCD=1`** |
| 颜色/对比异常 | 565 字节序或 RGB/BGR | 代码中 **`setSwapBytes(true)`**；尝试 **`TFT_RGB_ORDER=TFT_RGB`** 或 **`TFT_BGR`** |
| 未接 TF 时串口报 SD 错误 | `SD.begin()` 失败 | 第一阶段可忽略；接卡并按引脚表排查 |

---

## 5. 本阶段验收清单

- [ ] `python3 -m platformio run` 编译成功。
- [ ] 烧录后无 Guru 循环重启。
- [ ] 上电可见红 / 绿 / 蓝自检（或缩短后的快闪）。
- [ ] 启动画面可见 **色带 + 文字**（确认 `LOAD_GLCD` 生效）。
- [ ] 未接 TF 时允许串口出现 SD 相关错误；第二阶段再验收读卡。

---

## 6. 与第二阶段的衔接

下一阶段（你说「开始第二阶段」时可从此继续）通常包括：

1. **TF 卡**：按 `GPIO8` CS、共用 SPI、格式化 FAT32、`/roms` 与 ROM 列表读取。
2. **按键**：按 [pinout.md](pinout.md) 焊接或接入，验证菜单与去抖。
3. **GB/GBC 模拟核心**：在 `emulator/gbc_adapter` 中接入真实核心与主循环帧率。
4. **音频**：PWM 验证后，再 I2S + PCM5102。
5. **整机**：电源、续航、稳定性与回归清单。

---

## 7. 常用命令

```bash
cd "/Users/yangjianbo/Projects/EPS32-S3-GBA/firmware-ili9488-min"
python3 -m platformio run
python3 -m platformio run -t upload --upload-port /dev/cu.usbserial-XXXX
python3 -m platformio device monitor -b 115200 --port /dev/cu.usbserial-XXXX
```

将 `/dev/cu.usbserial-XXXX` 换为本机 `python3 -m platformio device list` 中实际端口。

---

## 8. 新手细读：概念、流程与每项功能在干什么

下面用「第一次做嵌入式」的视角，把上文里出现的名词和工程里的**功能**串起来讲一遍。不必一次全记住，需要时翻回对应小节即可。

### 8.1 先建立整体画面：三块东西在协作

| 角色 | 是什么 | 本阶段它干什么 |
|------|--------|----------------|
| **电脑（Mac）** | 写代码、编译、通过 USB 把程序送进单片机 | 运行 PlatformIO，生成 `.bin` 固件并烧录 |
| **ESP32-S3（开发板）** | 带 WiFi/蓝牙的 32 位微控制器，管脚叫 **GPIO** | 跑你的程序：通过 **SPI** 给屏幕发指令和像素数据 |
| **ILI9488 屏** | 一块带驱动芯片的 TFT，本方案用 **SPI** 通信 | 接收命令后点亮像素；需要 **供电、复位、数据、时钟、片选** 等信号 |

你可以把 ESP32 想成「小电脑」，屏是「只负责显示的显示器」；两者之间用几根线约定好的协议（SPI）说话。

---

### 8.2 几个必懂的名词（对应文档里会反复出现）

- **固件（Firmware）**：编译出来、烧进单片机 Flash 里的程序，上电就会自动运行。
- **烧录 / Upload**：把固件从电脑写入 ESP32 的存储芯片；通常用 USB 转串口（你机器上可能是 `usbserial` 或 `usbmodem`）。
- **GPIO**：通用输入输出引脚。程序里可以说「把 GPIO9 设为输出高电平」，屏的 **DC** 等脚就接在这些口上。
- **SPI（四线常用写法）**：一种常用的「主从」通信方式：
  - **SCK**：时钟，主控节奏；
  - **MOSI**：主控 → 从设备的数据（屏这边常叫 SDI）；
  - **MISO**：从设备 → 主控（屏若需要读回数据，走 SDO，接 **MISO**）；
  - **CS（片选）**：选中哪块芯片（屏一块、TF 卡一块，**共用** MOSI/SCK/MISO，靠不同 CS 区分）。
- **DC（或 RS）**：告诉屏「当前 SPI 传的是**命令**还是**数据**」，没有它屏会乱套。
- **RESET**：硬件复位，把屏内部状态机拉干净。
- **ILI9488**：屏幕内部驱动芯片型号；库和 `platformio.ini` 里写的 `ILI9488_DRIVER` 就是告诉 **TFT_eSPI**：按这款芯片的初始化序列发命令。
- **480×320**：分辨率；横竖屏由程序里的 **旋转**（rotation）决定。

---

### 8.3 PlatformIO 在本工程里「负责什么」

- **PlatformIO**：在 VS Code / Cursor 里常用的嵌入式工程工具（类似「专门给单片机用的项目管理」）。
- **`platformio.ini`**：工程的**总配置**——用哪块板卡、用哪个框架（这里是 **Arduino**）、要拉哪些库（如 **TFT_eSPI**）、以及一堆 **`-D xxx=yyy`** 的**编译宏**。
- **编译宏（`-D ...`）**：在编译前告诉库「屏幕是几号驱动、引脚接在哪、用哪路 SPI」等；**不改源码**也能改硬件适配，所以第一阶段很多关键修复都写在这里。

**为什么要写 `python3 -m platformio`？**  
有时图形界面里的 PlatformIO 插件卡住，但系统里已通过 pip 安装的 `platformio` 命令仍可用；用终端走同一套流程，不依赖插件是否初始化成功。

---

### 8.4 TFT_eSPI 是干什么的？和 `platformio.ini` 里那些宏的关系

**TFT_eSPI** 是一个 Arduino 库，帮你：

- 把「画线、画矩形、写字、刷整屏颜色」变成对屏幕驱动芯片的**命令 + 像素数据**；
- 在 ESP32 上选好 **SPI 外设**，把 MOSI/SCLK/CS/DC/RST 接到对应 GPIO。

本工程里通过宏告诉它（示例含义，具体以你仓库里的 `platformio.ini` 为准）：

| 宏 | 对新手的大白话 |
|----|----------------|
| `USER_SETUP_LOADED` | 「我不使用库自带的默认 `User_Setup.h`，引脚和选项都在 PlatformIO 里写」 |
| `ILI9488_DRIVER` | 「驱动芯片是 ILI9488」 |
| `TFT_MOSI` / `TFT_SCLK` / `TFT_CS` / `TFT_DC` / `TFT_RST` | 「屏的这根脚接到 ESP32 的哪个 GPIO」 |
| `TFT_MISO` | 「读回数据用的脚（接屏的 SDO）」——S3 上若缺省，库可能把 MISO 与 MOSI 绑死，导致 SPI 初始化失败（你遇到的 Guru） |
| `USE_HSPI_PORT` | 「用另一路 SPI 主机（与默认 FSPI 区分）」——用于规避部分板型与库组合下的兼容问题 |
| `SPI_FREQUENCY` | SPI 时钟多快；面包板线长容易干扰，**降频**往往更稳 |
| `TFT_RGB_ORDER` | 像素红蓝顺序；不对时颜色会怪，可 RGB/BGR 互换试 |
| `LOAD_GLCD` | 「把内置点阵字体**编进固件**」——不加则字模为空，**只有色块没有字**（你遇到的现象） |

代码里 **`setSwapBytes(true)`** 表示：把 16 位颜色（RGB565）的高低字节按屏/主机期望的方式交换，否则颜色或文字可能异常。

---

### 8.5 程序上电后大致在做什么（对应 `main.cpp` 思路）

1. **上电**：Bootloader 从 Flash 读你的固件，从 `setup()` 开始执行。
2. **`setup()`**（只做一次）：
   - 开串口（方便你在电脑上 `monitor` 看打印和报错）；
   - **`g_display.begin()`**：初始化 SPI + 屏驱动（TFT_eSPI），里面可能有 **红绿蓝自检**、再画启动画面；
   - 初始化按键、音频（本阶段可先不关心）；
   - **`g_storage.begin()`**：尝试挂载 TF；**没插卡**时这里会失败，串口报错——**第一阶段可以忽略**；
   - 根据是否读到卡，刷新菜单或「SD 失败」提示。
3. **`loop()`**（反复执行）：
   - 读按键、刷新菜单、若以后有游戏则跑模拟器帧循环。

所以：**屏能亮、能变色、能出字**，说明 `display` 驱动和 `setup` 里显示相关部分基本正确；**SD 报错**只说明存储子系统还没接好或没接卡。

---

### 8.6 工程里每个「文件夹/文件」在干什么（按功能理解）

| 位置 | 功能（新手理解） |
|------|------------------|
| `boards/pin_config.h` | **一张表**：把「方向键、A 键、TF CS」等名字和 GPIO 号对应起来，改接线时主要改这里（和 `platformio.ini` 里屏相关宏保持一致）。 |
| `drivers/display_ili9488.*` | **屏的专用封装**：`begin()` 里做初始化、自检、启动画面；`drawText()` 等给上层用。 |
| `drivers/storage_sd.*` | **读 TF 卡**：挂载 SD、列目录、读 ROM 文件。 |
| `drivers/input_keys.*` | **读按键**：扫描 GPIO，带简单去抖。 |
| `app/rom_menu.*` | **ROM 菜单逻辑**：上下选 ROM、确认等。 |
| `emulator/gbc_adapter.*` | **以后接模拟器核心**用的适配层；第一阶段可以是占位或简单测试画面。 |
| `main.cpp` | **把上面全部串起来**：先硬件，再菜单，再（将来）游戏循环。 |

---

### 8.7 本阶段「必须搞懂的三个坑」（对应第 4 节表格）

1. **SPI 初始化失败（Guru）**：不是屏坏了，多半是 **MISO 引脚与库默认不一致** 或 **SPI 主机选择** 问题；用 `TFT_MISO` + 必要时 `USE_HSPI_PORT` 解决。
2. **只有色块没有字**：不是字库坏了，是 **没开 `LOAD_GLCD`**，库用空字模，打印出来的笔画是「空的」。
3. **颜色不对**：先试 **`setSwapBytes`**，再试 **`TFT_RGB_ORDER`**；面包板线长时还可 **降低 SPI 频率**。

---

### 8.8 引脚为什么要「这样」接（原理拆开讲）

先分清两件事：**物理上必须满足的**，和**本项目人为约定、但必须和代码一致**的。

#### （1）两类信号：SPI 总线脚 vs 普通控制脚

| 类型 | 屏上常见名字 | 作用 |
|------|--------------|------|
| **SPI 数据线** | SDI/MOSI、SCK、SDO/MISO | 高速串行传命令和像素；**必须**接到 ESP32 上**同一套 SPI 外设**能使用的那些脚（或由 GPIO 矩阵复用到该外设），且与 `platformio.ini` 里 `TFT_MOSI` / `TFT_SCLK` / `TFT_MISO` **完全一致**。 |
| **普通 GPIO** | CS、DC、RST（有的屏叫 RS） | 不要求走 SPI 硬件的「数据通道」，**任意空闲 GPIO** 都可以，只要程序里写的是几号脚、杜邦线就连几号脚。 |

所以：**11 / 12 / 13** 这组不是「宇宙真理」，而是本工程为 ESP32-S3 选的 **FSPI（SPI2）常用引出脚** + 与 **TFT_eSPI**、**Arduino-ESP32** 组合验证过的一套；**9 / 10 / 14** 则是为避开 USB、Strapping、以后按键/TF 等，在面包板阶段**冻结下来**的编号——**真正关键的是「线 ↔ 宏定义 ↔ pin_config」三者一致**，而不是某个数字本身神秘。

#### （2）每一根线「为什么要存在」

- **VCC / GND**：屏内驱动芯片和 TFT 面板要工作电压与参考地；不接或接触不良会完全不亮或乱闪。
- **MOSI（接 GPIO11）**：主控**输出**到屏的所有串行位都走这根（命令寄存器值、显存里的像素数据都靠它）。
- **SCK（接 GPIO12）**：时钟；屏在时钟边沿采样 MOSI，速度由 `SPI_FREQUENCY` 等限制，面包板线长时**降频**更稳。
- **MISO（接 GPIO13，接屏的 SDO）**：主控**读回**时用的线。ILI9488 在初始化或读 ID、读像素等场景会**从屏往外送数据**；若这根没接或接到别的脚，SPI 外设状态与库内部假设不一致，容易出现你遇到过的 **`SPI.beginTransaction` / Guru**。本工程显式写 **`TFT_MISO=13`**，就是让库和硬件**同一套「读通道」**。
- **CS（接 GPIO10）**：**片选**。同一根 MOSI/SCK 上还会挂 TF 卡（见下），谁被选中由 **谁的 CS 拉低** 决定；不拉低屏的 CS，屏不应答。
- **DC（接 GPIO9）**：**命令 / 数据选择**。同一根 MOSI 上既传「写寄存器命令」又传「寄存器参数或像素流」；DC 的电平告诉 ILI9488 当前字节算**命令**还是**数据**。缺 DC 或接错脚，屏会解析乱套，表现常为花屏、白屏、无反应。
- **RST（接 GPIO14）**：**硬件复位**。上电或初始化前把屏内部状态机拉清楚；有些屏不接 RST 偶尔也能亮，但不可靠，工程上应接上。
- **LED / 背光**：背光本质是**另一路供电或 PWM**；你可用 **LED 接 3V3 常亮**，此时固件里 [`pin_config.h`](../firmware-ili9488-min/src/boards/pin_config.h) 中 **`kBacklightOnGpio = false`**，避免再去用 GPIO21 拉高（避免误以为要用 21 却没接而困惑）。

#### （3）TF 卡 CS 为什么是 GPIO8，而不能「随便用 13」

屏与 TF **共用** MOSI、SCK、MISO（**一条 SPI 总线**），靠 **不同的 CS** 区分设备：

- **GPIO13** 在本工程里是 **SPI 的 MISO**（总线读回线），必须留给屏的 SDO（以及 TF 的 DO），**不能再兼作某设备的片选**。
- 因此 TF 的片选单独用 **GPIO8**，与 `pin_config.h` 里 `kSdCs = 8` 一致。

#### （4）和 `USE_HSPI_PORT` 的关系（简要）

在部分 ESP32-S3 + TFT_eSPI 组合下，默认 SPI 端口与内部 Flash、外设路由混在一起时，初始化会踩坑。启用 **`USE_HSPI_PORT`** 是让库改用另一路 SPI 主机端口，与你在 `platformio.ini` 里指定的 **MOSI/MISO/SCLK** 配置配套，从而**稳定完成 `tft_.init()`**。这是**软件与芯片外设路由**层面的选择，不是杜邦线颜色问题。

---

### 8.9 为什么现在能显示「画面」和「文字」（从代码到像素）

可以按时间顺序理解：**先能通信 → 再能填色 → 再能画字**。

#### 步骤 A：能通信（`tft_.init()`）

`TFT_eSPI` 里的 `init()` 会对 ILI9488 发一串**厂家规定的上电/休眠退出/像素格式/扫描方向**等命令（datasheet 里的初始化序列）。只有 **SPI 引脚、DC、RST、CS** 都对、**MISO** 配置正确，这次序列才会成功，屏进入「可写显存」状态。否则会卡死或 Guru——**不是「显示驱动魔法」，而是总线先通了**。

#### 步骤 B：「画面」= 往显存里写颜色（几何与色块）

你看到的红 / 绿 / 蓝自检、色带、黑底，在代码里对应的是 [`display_ili9488.cpp`](../firmware-ili9488-min/src/drivers/display_ili9488.cpp) 里的 `fillScreen`、`fillRect` 等：

- 本质是：库把 **RGB565**（每个像素 16 位）按当前设置的扫描方向，**连续写入屏内显存对应地址窗口**。
- **`setRotation(1)`**：决定 480×320 是横屏还是竖屏、行列从哪一角递增。
- **`setSwapBytes(true)`** 与 **`TFT_RGB_ORDER`**：对齐**主机字节序**与屏对 **R/G/B 顺序** 的约定；不对则整体偏色或发糊。
- **`invertDisplay`**：是否反相显示，按屏批次可试。

所以：**「有画面」= SPI 写像素成功 + 颜色格式与旋转设置大致正确**。

#### 步骤 C：「文字」= 字模点阵 + 当作小图片画上去

`println("ESP32-S3 OK")` 这类调用**不是**屏自带字体，而是 **TFT_eSPI 用内置点阵字体**（GLCD 字体）把每个字符变成一小块 **位图**，再逐像素用前景色/背景色画进帧缓冲对应区域——和画矩形是同一套「写像素」能力。

- 工程里开启了 **`-D LOAD_GLCD=1`**：才会把这套**默认字模数据**编进固件；若关闭，字模表为空，就会出现**只有色块、没有笔画**的现象。
- 文字大小 `setTextSize`、颜色 `setTextColor`、位置 `setCursor`，都只是**控制这块位图画在哪、多大、什么颜色**。

因此：**「有文字」= 初始化成功 + 能写像素 + `LOAD_GLCD` 提供字模数据**；二者缺一不可。

#### 小结成一句话

- **接线**：让 SPI（含 MISO）与 CS/DC/RST 在硬件上与程序一致，屏才能被正确初始化并接收像素流。  
- **画面**：向显存写 RGB565。  
- **文字**：用 GLCD 字模把字符栅格化成像素再写进去；**`LOAD_GLCD=1`** 负责把字模带进固件。

---

### 8.10 你接下来可以怎么「研究」这份文档

1. 对照 [pinout.md](pinout.md) 在实物上**一根线一根线**核对。  
2. 打开 [platformio.ini](../firmware-ili9488-min/platformio.ini)，每行 `-D` 对照 **8.4 节表** 想一遍。  
3. 打开 [display_ili9488.cpp](../firmware-ili9488-min/src/drivers/display_ili9488.cpp)，看 `begin()` 里顺序和 `showSplash()` 里画了啥。  
4. 用串口监视器看 **SD 报错** 是否仍出现（没接卡时正常）。

当你说「开始第二阶段」时，一般会从 **TF 接线 + `storage_sd` 验证 + 菜单里出现 ROM 列表** 继续往下做。
