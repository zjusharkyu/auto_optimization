# 🔬 实验日志 (Experimental Log)

| # | 描述 | MEDIAN_SCORE | MIN_SCORE | 结果 |
|---|------|-------------|-----------|------|
| 0 | 基线 (Baseline) | 56.69 | 52.18 | - |
| 1 | 缓存 order.symbol 到局部变量 | 53.80 | 52.57 | ✅ Keep |
| 2 | 缓存 order.trader 到局部变量 | 54.35 | 52.16 | ❌ Discard |
| 3 | limit 参数改为 const reference | 54.91 | 52.75 | ❌ Discard |
| 4 | vector 添加 reserve | 54.66 | 52.25 | ❌ Discard |
| 5 | 缓存 hotPathVars 到局部变量 | 55.59 | 54.63 | ❌ Discard |
| 6 | 优化 pop_front 循环使用 erase_after | 53.41 | 52.44 | ✅ Keep |
| 7 | 使用原始指针访问 arenaBookEntries | 68.03 | 54.11 | ❌ Discard |
| 8 | 缓存 bookEntry->trader 到局部变量 | 53.70 | 51.98 | ❌ Discard |
| 9 | 进一步优化 pop_front 循环 | 53.62 | 52.22 | ❌ Discard |
| 10 | 添加 __restrict__ 关键字 | 54.03 | 52.01 | ❌ Discard |
| 11 | 将 exec 结构体声明到函数开始 | 55.04 | 52.49 | ❌ Discard |
| 12 | cancel 函数使用原始指针 | 54.00 | 51.65 | ❌ Discard |
| 13 | 缓存 bookEntry->end() (不可行) | - | - | ❌ Discard |
| 14 | 使用前缀递增优化 ppEntry++ | 54.26 | 51.90 | ❌ Discard |
| 15 | 优化 pop_front 循环使用迭代器 | 53.80 | 52.38 | ❌ Discard |
| 16 | push_front 替代 push_back (失败) | - | - | ❌ Discard |
| 17 | 优化 HotPathVars 成员顺序 | 57.67 | 53.20 | ❌ Discard |
| 18 | 使用 data() 避免调用 begin() | 54.26 | 52.57 | ❌ Discard |
| 19 | 改回 pop_front (更慢) | 54.16 | 52.46 | ❌ Discard |
| 20 | 预计算 curOrderID | 55.16 | 53.30 | ❌ Discard |