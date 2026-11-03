# 電影神槍手反應模擬器 (Gunfighter Reaction Simulator)
**STM32G474RE 嵌入式系統期中專題完整工程報告**

---

## 1. 專題資訊 (Cover & Basic Info)
* **專題名稱**：電影神槍手反應模擬器
* **學生姓名**：林士璿
* **學號**：B11353019
* **硬體平台**：NUCLEO-G474RE
* **核心技術**：EXTI + TIM2 + USART2
* **GitHub 專案網址**：`https://github.com/your-id/gunfighter-reaction-simulator` *(請依實際 GitHub 帳號修訂)*

---

## 2. 情境故事與開發動機 (Story & Motivation)

### 2.1 現狀與痛點
每次觀看西部片主角在生死交關時瞬間拔槍對決，總令人熱血沸騰。然而在現實中，人們無法精確得知自身的反射神經極限與反應速度（ms 毫秒等級）。

### 2.2 裝置解法
本專題利用 STM32 模擬西部決鬥號令情境：
1. **進入預備**：隨機等待 1～4 秒（黃燈亮起）。
2. **決鬥號令**：瞬間點亮綠燈並啟動毫秒級計時器。
3. **生死拔槍**：玩家按下板上 B1 按鍵作為扳機，精準記錄反應時間。

### 2.3 規則機制
* **搶跑/偷跑懲罰**：若號令燈（綠燈）亮起前即按下按鍵，系統判定違規並亮紅燈提示 `Early/Fail`。
* **逾時懲罰**：若綠燈亮起後超過 1 秒未拔槍，自動判定失敗並亮紅燈提示 `Timeout`。
* **精準回報**：成功拔槍則捕捉 $1\text{ ms}$ 誤差內的反應速度，並透過雲端/串口終端機即時回報。

---

## 3. 硬體對應與腳位配置 (Hardware Alignment & Pin Map)

### 3.1 故事世界與硬體替身 (Hardware Replacement)
* **決鬥號令三色燈**：`PA5`（綠燈/開火）、`PA6`（黃燈/預備）、`PA7`（紅燈/犯規或逾時）
* **決鬥感應扳機**：板上使用者藍色按鍵 B1（`PC13`），觸發 EXTI 中斷。
* **生死對決計時器**：`TIM2`（170MHz 主頻，$\text{PSC}=169, \text{ARR}=999$，每 $1\text{ ms}$ 產生一次中斷）。
* **戰績雲端榜**：`USART2`（波特率 115200, 8N1），透過 Serial 終端機即時輸出成績與歷史最佳紀錄。

### 3.2 NUCLEO-G474RE 實體腳位分配表
| MCU 腳位 (Pin) | 硬體替身元件 | 工作模式 / 暫存器配置 | 用途描述 |
| :--- | :--- | :--- | :--- |
| **PA5** | 綠色單色 LED (板上 LD2) | GPIO Output (Push-Pull) | 拔槍號令燈（亮起瞬間開啟 TIM2 計時） |
| **PA6** | 黃色單色 LED | GPIO Output (Push-Pull) | 預備警戒燈（進入 1~4 秒隨機倒數） |
| **PA7** | 紅色單色 LED | GPIO Output (Push-Pull) | 犯規/逾時燈（偷跑或超過 1 秒未拔槍） |
| **PC13** | 板上藍色按鍵 (B1) | EXTI Line13 (Falling Edge) | 生死扳機按鍵（觸發捕獲反應時間） |
| **PA2 / PA3** | ST-LINK Virtual COM Port | USART2 AF (TX/RX) | 電腦終端機傳輸（115200 Baud rate） |

---

## 4. 實作照片與情境證明 (Hardware Implementation Photos)

以下為系統於各狀態下之實體電路接線與測試驗收照片：

### 4.1 搶跑/犯規或逾時狀態 (Red Light Alert)
* **狀態說明**：當玩家於黃燈預備期提前按下 B1（偷跑），或綠燈亮起後超過 1 秒未按鍵（逾時），系統熄滅其他燈號並點亮紅色 LED。
* **實體照片**：
  ![紅色 LED 亮起 - 違規/逾時](red.jpg)

### 4.2 預備警戒狀態 (Yellow Light Waiting)
* **狀態說明**：系統收到 `new` 命令或觸發重置後，點亮黃色 LED，並進行 1～4 秒隨機時間等待。
* **實體照片**：
  ![黃色 LED 亮起 - 預備等待](yellow.jpg)

### 4.3 綠燈開火與扳機觸發 (Green Light & Trigger Input)
* **狀態說明**：隨機等待結束後點亮綠色 LED，TIM2 開始計時；玩家按下板上 B1 按鍵（扳機），觸發 PC13 下降沿中斷並計算反應時間。
* **實體照片**：
  ![綠色 LED 亮起 - 開火號令](green.jpg)
  ![按下 B1 按鍵 - 觸發中斷](image_dcb3d0.png)

---

## 5. 系統架構與狀態轉換 (System Architecture & State Machine)

### 5.1 狀態轉換邏輯
1. **IDLE / PREPARE 狀態**：
   * 終端機輸入 `new` 或按下 B1 重新啟動。
   * 亮黃燈（`PA6=1`），關閉紅綠燈。
   * 啟動隨機倒數 $T_{\text{wait}} \in [1000\text{ ms}, 4000\text{ ms}]$。
2. **EARLY TRIGGER 判斷**：
   * 若於 $T_{\text{wait}}$ 期間檢測到 `PC13`（B1 按下），轉入 **FAIL 狀態**。
   * 熄黃燈，亮紅燈（`PA7=1`），串口傳送 `"Early Trigger! Fail!"`。
3. **FIRE / TIMING 狀態**：
   * $T_{\text{wait}}$ 時間到，熄黃燈，亮綠燈（`PA5=1`），啟動 TIM2 毫秒累加計數。
4. **SUCCESS / TIMEOUT 判斷**：
   * 若於 $1000\text{ ms}$ 内按下 B1，記錄當前時間 $T_{\text{reaction}}$，轉入 **SUCCESS 狀態**，串口輸出 `"Reaction Time: xxx ms, SUCCESS!"`。
   * 若超過 $1000\text{ ms}$ 未按下，轉入 **FAIL 狀態**，亮紅燈（`PA7=1`），串口輸出 `"Timeout! Fail!"`。

---

## 6. 專題驗收條件與實測結果 (Measurable Checklist)

| 驗收項目 | 測試動作 | 預期結果 | 實際驗收結果 | 判定 |
| :--- | :--- | :--- | :--- | :--- |
| **1. 預備啟動驗收** | 終端機輸入 `new` 或按下 B1 | 黃燈即時亮起，進入 1~4 秒隨機倒數狀態 | 黃燈亮起，隨機等待延時正常 | **PASS** |
| **2. 偷跑/犯規判定** | 隨機等待結束（綠燈亮）前按下 B1 | $0.1\text{ s}$ 內轉紅燈，終端機輸出 `Early/Fail` | 即時轉紅燈，終端機印出 `Early Trigger!` | **PASS** |
| **3. 正常拔槍計時** | 綠燈亮起後 $1\text{ s}$ 內按下 B1 | 綠燈滅，輸出 `Reaction Time: xxx ms, SUCCESS` | 成功紀錄毫秒時間（如 $235\text{ ms}$） | **PASS** |
| **4. 逾時未拔槍** | 綠燈亮起後超過 $1\text{ s}$ 未按 B1 | 自動轉紅燈，終端機顯示 `Timeout` | 超過 $1000\text{ ms}$ 自動切換紅燈並警告 | **PASS** |

---

## 7. 技術勾選表 (Technical Checklist)

本專題實際運用之週次技術與暫存器配置如下：

* [x] **GPIO 輸出**：紅綠黃燈：`PA5` (綠), `PA6` (黃), `PA7` (紅) \| `MODER = 01` \| `BSRR` 設 / 清
* [x] **GPIO 輸入**：B1 按鍵拔槍 \| `PC13` \| `MODER = 00` \| EXTI 的來源腳
* [x] **複用功能 AF**：上傳串口資料：USART2 收發 \| `PA2 / PA3` \| `MODER = 10` \| `AFR` 選 `AF7`
* [x] **時脈**：PLL 170 MHz \| `HSI16` 主時脈基準 \| 系統運作與計時基準
* [x] **外部中斷 EXTI**：B1 一按 = 扣扳機 \| `EXTICR4` 選 `PC13` \| `FTSR1` (下降沿觸發) \| `IMR1` \| `PR1` 寫 1 清除
* [x] **NVIC 致能**：三支 ISR 均配置中斷服務 \| `TIM2`, `EXTI15_10`, `USART2` 三個 IRQ
* [ ] **優先權與巢狀**：未額外設定優先權階層
* [x] **軟體去彈跳**：按鍵去彈跳處理 \| 時間戳：$\text{ticks} - t_{\text{btn}} \ge 50\text{ ms}$ 才算一次有效按壓
* [x] **UART 傳送**：成績上報：每次事件 `report()` 一行 \| USART2 115200 8N1 \| `BRR = 1476` \| 輪詢 `TXE`
* [x] **UART 接收 + 環形緩衝**：命令接收：`new` 指令解析 \| `RXNE` 中斷入列 \| 主迴圈出列解析
* [x] **Timer 定時中斷**：計時週期 $1\text{ ms}$ \| `TIM2 PSC = 169`, `ARR = 999` \| $1\text{ ms}$ `UIF` 溢位中斷，累加毫秒數
* [ ] **PWM**：本專題未用到 PWM
* [ ] **輸入捕捉**：本專題未用到 Timer Input Capture

---

## 8. AI 揭露與開發查核說明 (AI Disclosure & Checklist)

### 8.1 AI 工具協助範圍
* 專題整體 C 語言狀態機架構與邏輯擬定。
* `TIM2` 時脈分頻係數（$\text{PSC} = 169, \text{ARR} = 999$）算式驗證與生成。
* `USART2` 格式化字串（`sprintf` / `printf`）輸出語法優化。

### 8.2 開發者親自查核事項
* **查核 RM0440 Reference Manual**：驗證 STM32G474 在 $170\text{ MHz}$ 時脈下 `TIM2` $1\text{ ms}$ 計數精準度。
* **實測 EXTI13 (PC13)**：按鍵機械彈跳時間，經實測加入 $50\text{ ms}$ 軟體去彈跳機制。
* **實測 GPIO 電壓**：實測三顆 LED 腳位（`PA5/PA6/PA7`）推挽輸出電壓（$3.3\text{ V}$）與對應限流電阻。