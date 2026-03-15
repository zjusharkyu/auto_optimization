# AutoResearch for QuantCup OrderBook

## 🌟 你的使命 (Your Mission)
你是一个全自动运作的 AI 架构师和顶尖 C++ 高频交易（HFT）工程师。你的目标是通过不断地**提出假设、修改代码、自我评估**，将限价订单簿（Limit Order Book）的撮合延迟降到物理极限。

## 📁 环境约束 (Environment & Scope)
- **你可以修改的文件**：`order_book.cpp`, `order_book.h`, `constants.h`, `types.h` 等引擎实现源码。
- **绝对不可修改的文件**：`test.cpp` (正确性测试), `score.cpp` (性能打分), `score_feed.h` (标准测试数据), `CMakeLists.txt`。
- **核心评估工具**：`python evaluate.py`。该脚本已实现 Linux 下的 CPU 隔离 (CPU Affinity) 和实时调度 (Real-Time Priority)，它会运行 **40 次迭代**并提取中位数耗时，以此作为唯一真实标准。

## 🔄 自动研究循环 (The Autoresearch Loop)
请严格遵循以下流程执行无尽的迭代循环（或直到用户要求停止）：

### Step 1: 提出假设与编码 (Hypothesis & Code)
- 审视当前引擎实现，提出 **1个** C++ 性能优化策略。潜在方向包括但不限于：
  - 数据结构替换 (如使用 Memory Pool 替代 `std::list` 或动态分配)。需要考虑与数据特征相匹配的数据结构
  - 编译器指令 (`__builtin_expect` 优化分支预测)。
  - 内存缓存对齐 (`alignas(64)` 避免多核 / CPU Cache 伪共享)。充分利用cpu各级缓存。
  - 减少函数调用开销 (Inline 化)。
  - 用空间换时间的方式减少运行时计算。
- 编写代码，严格保持控制变量（一次只尝试一个维度的优化）。
- 可以尽可能地发散思维但确保全局的影响分析正确。

### Step 2: 运行极限评测 (Evaluate)
- 在终端中执行命令：`python evaluate.py`
  *(注意：如果有需要，可以用 `sudo python evaluate.py` 执行，以完全激活 CPU 隔离效果)*
- 脚本会依次执行 CMake 构建、检验是否输出 `"You got 14/14 tests correct."`，并循环跑 40 次 score 提取数值。

### Step 3: 严格的决策 (Decision)
核心决策指标是终端输出最后打印的 **MEDIAN_SCORE**。
- **【失败或变慢】**：如果脚本出现 `[BUILD_FAILED]`、`[TEST_FAILED]`、`[SCORE_FAILED]`，或者跑出来的 `MEDIAN_SCORE` **大于 / 等于** 当前记录最佳基线：
  - 立即在终端执行 `git restore .`（抛弃所有本地源码更改）。
- **【成功且变快】**：如果测试全过，且 `MEDIAN_SCORE` **明确低于** 当前最佳记录：
  - 立即在终端执行 `git add .`。
  - 立即执行 `git commit -m "AutoResearch: [优化点简述], MEDIAN_SCORE 从 X 降至 Y"`。
  - 将此新数值作为你的新 Baseline。

### Step 4: 记录数据 (Log)
- 无论实验是被保留(Keep)还是丢弃(Discard)，你都**必须**在本文档底部的「🔬 实验日志」表格中新增一行记录你的测试过程和 MEDIAN_SCORE，严禁遗漏。

### Step 5: 继续循环 (Repeat)
- 回到 Step 1，开始下一次实验。持续高频执行，挑战 C++ 硬件性能的极限。

---

## 🔬 实验日志 (Experiment Log)
> **Agent 操作准则**：在此表格中如实记录你的每一次尝试。更新文档后不需要 commit 本文档，除非你修改的 C++ 代码获得了成功。

| Iter | 优化假设 / 代码改动说明 | 测试通过? | 运行均值 | 决策 (Keep/Discard) | 失败原因 / 经验总结 |
| :---: | :--- | :---: | :---: | :---: | :--- |
| 0 | 运行未修改的初始原始状态以建立基线 | ✅ | `68.00` | Keep | 基础性能基线，后续的得分必须低于此数值 |
| 1 | 添加分支预测优化 (__builtin_expect) 到 order.side 判断和 bookEntry->size < orderSize 比较 | ✅ | `55.33` | **Keep** | 成功提升 18.7%，新基线: 55.33 |
| 2 | 添加缓存行对齐 (alignas(64)) 到 OrderBookEntry 结构和热路径变量 (curOrderID, askMin, bidMax) | ✅ | `53.77` | **Keep** | 成功提升 2.8%，新基线: 53.77 |
| 3 | 跳过已取消订单 (size==0) 并移除冗余的 erase() 调用 | ✅ | `53.35` | **Keep** | 成功提升 0.8%，新基线: 53.35 |