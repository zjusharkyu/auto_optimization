# 🔬 实验日志 (Experimental Log)

| # | 描述 | MEDIAN_SCORE | MIN_SCORE | 结果 |
|---|------|-------------|-----------|------|
| 0 | 基线 (Baseline) | 56.69 | 52.18 | - |
| 1 | 缓存 order.symbol 到局部变量 | 53.80 | 52.57 | ✅ Keep |
| 2 | 缓存 order.trader 到局部变量 | 54.35 | 52.16 | ❌ Discard |
| 3 | limit 参数改为 const reference | 54.91 | 52.75 | ❌ Discard |
| 4 | vector 添加 reserve | 54.66 | 52.25 | ❌ Discard |
| 5 | 缓存 hotPathVars 到局部变量 | 55.59 | 54.63 | ❌ Discard |