## 1. 实验目标

本实验比较了大作业中手写红黑树 `ESet` 与 `std::set` 的性能差异，并基于 baseline 实现相比 STL 的不足，通过消融实验分析了两个进一步工程优化的效果：

1. inline value：将节点中的 `Key*` 改为节点内部直接存储 `Key`，减少一次间接访问和一次独立堆分配。
2. memory pool：为普通数据节点实现 chunk-based 节点池，减少小对象频繁 `new/delete` 的开销。

需要强调的是，`ESet` 并不只是复刻 `std::set`。它额外在每个节点维护 subtree size，因此可以支持 `range(l, r)` 的 $O(\log n)$ 区间计数。相比之下，`std::set` 没有 order-statistics API，本实验中的 STL baseline 只能通过 `lower_bound(l)`、`upper_bound(r)` 后再 `distance` 遍历区间来完成相同功能。因此，为了避免 range 过分影响整体时间比例，我们在主实验中把 range 测试的大小设置的比较保守，并单独设计了一个对不同规模 range 速度进行检测的附加实验。

---

## 2. 实验设置

### 2.1 Mixed Benchmark

mixed benchmark 为主实验，测量我们实现的 `ESet` 的综合性能。

参数：

| item             |                                                      value |
| ---------------- | ---------------------------------------------------------: |
| ops per trace    |                                                     200000 |
| sets             |                                                         25 |
| repeats per seed |                                                          3 |
| seeds            | 2026051601, 2027051604, 2028051607, 2029051610, 2030051613 |

> 这里的 sets 即每次实验维护一个大小为 25 的 set 数组，每次操作在这些 set 上随机进行

每次测试的操作与比例如下：

| operation     | count per trace |   proportion |
| ------------- | --------------: | -----------: |
| `emplace`     |          100000 |      50.000% |
| `erase`       |           33333 |      16.667% |
| `find`        |           22222 |      11.111% |
| `range`       |           11111 |       5.556% |
| `lower_bound` |            5551 |       2.776% |
| `upper_bound` |            5551 |       2.776% |
| `++it`        |            5551 |       2.776% |
| `--it`        |            5551 |       2.776% |
| `assignment`  |              25 |       0.013% |
| `begin`       |            5551 |       2.776% |
| `end`         |            5554 |       2.777% |
| **total**     |      **200000** | **100.000%** |

为了保证可比性，每个 seed 只生成一次 trace，`std::set` 与三个 `ESet` 共用完全相同的操作序列。

测试的三个 `ESet` variant 为：

| variant                    | description                |
| -------------------------- | -------------------------- |
| baseline                   | 原始实现，节点中存 `Key*`，节点和值均独立分配 |
| inline value               | 节点内部直接存储 `Key`             |
| inline value + memory pool | 在 inline value 基础上启用节点池    |
> 注：代码没有为 no inline value 单独设计 memory pool 实现，因为其意义不大且比较麻烦

### 2.2 Range Sweep

range sweep 用于单独评估区间计数性能。它在固定数据集上测试三种区间跨度：

| operation | range span |
|---|---|
| `range_small` | $r-l \in [0, 100]$ |
| `range_medium` | $r-l \in [0, 10000]$ |
| `range_large` | $r-l$ 覆盖接近全 key range 的大区间 |

这组实验用于观察区间内元素数量 $k$ 增大时，`std::set` 的 $O(\log n + k)$ 方案与 `ESet` 的 $O(\log n)$ rank 方案之间的差异。

---

## 3. Mixed Benchmark 结果

本节比较三种 `ESet` 实现：

| notation | setting                    | meaning                                     |
| -------- | -------------------------- | ------------------------------------------- |
| `S0`     | baseline                   | `ESET_INLINE_VALUE=0`, `ESET_MEMORY_POOL=0` |
| `S1`     | inline value               | `ESET_INLINE_VALUE=1`, `ESET_MEMORY_POOL=0` |
| `S2`     | inline value + memory pool | `ESET_INLINE_VALUE=1`, `ESET_MEMORY_POOL=1` |

其中 `STL` 表示 `std::set`。所有耗时单位均为 ns/op。
### 3.1 Raw Runtime

| operation | STL | S0 | S1 | S2 |
|---|---:|---:|---:|---:|
| total | 224.16 | 290.64 | 287.43 | 225.86 |
| emplace | 221.92 | 304.02 | 303.06 | 258.93 |
| erase | 163.37 | 218.52 | 207.77 | 151.06 |
| find | 172.03 | 224.14 | 212.39 | 155.06 |
| range | 421.08 | 272.67 | 272.15 | 206.56 |
| lower_bound | 167.00 | 218.57 | 215.18 | 157.80 |
| upper_bound | 168.80 | 222.61 | 204.42 | 158.65 |
| ++it | 38.74 | 40.94 | 42.23 | 39.04 |
| --it | 36.87 | 42.44 | 43.57 | 44.61 |
| assignment | 64011.80 | 174350.67 | 186423.82 | 55282.02 |
| begin | 33.12 | 34.16 | 30.90 | 30.97 |
| end | 25.01 | 29.04 | 24.36 | 24.97 |
### 3.2 Relative Performance

前三列为相比 STL，不同实现的耗时比例，而后三列则为两个工程优化相比 baseline 实现的加速比

| operation   | S0/STL | S1/STL | S2/STL | S0/S1 | S1/S2 | S0/S2 |
| ----------- | -----: | -----: | -----: | ----: | ----: | ----: |
| total       |   1.30 |   1.28 |   1.01 |  1.01 |  1.27 |  1.29 |
| emplace     |   1.37 |   1.37 |   1.17 |  1.00 |  1.17 |  1.17 |
| erase       |   1.34 |   1.27 |   0.92 |  1.05 |  1.38 |  1.45 |
| find        |   1.30 |   1.23 |   0.90 |  1.06 |  1.37 |  1.45 |
| range       |   0.65 |   0.65 |   0.49 |  1.00 |  1.32 |  1.32 |
| lower_bound |   1.31 |   1.29 |   0.94 |  1.02 |  1.36 |  1.39 |
| upper_bound |   1.32 |   1.21 |   0.94 |  1.09 |  1.29 |  1.40 |
| ++it        |   1.06 |   1.09 |   1.01 |  0.97 |  1.08 |  1.05 |
| --it        |   1.15 |   1.18 |   1.21 |  0.97 |  0.98 |  0.95 |
| assignment  |   2.72 |   2.91 |   0.86 |  0.94 |  3.37 |  3.15 |
| begin       |   1.03 |   0.93 |   0.94 |  1.11 |  1.00 |  1.10 |
| end         |   1.16 |   0.97 |   1.00 |  1.19 |  0.98 |  1.16 |

### 3.3 总体表现

baseline 版本的总耗时为 `290.64 ns/op`，约为 `std::set` 的 `1.30x`。这说明原始手写红黑树在常数上仍明显落后于 STL。

inline value 后，总耗时为 `287.43 ns/op`，相对 baseline 的总 speedup 为 `1.01x`，在本次实验中总体收益并不明显。它确实减少了 `Key*` 间接访问和 `Key` 单独分配，但该收益被节点分配、subtree size 维护、树路径访问等其它成本部分抵消。

加入 memory pool 后，总耗时下降到 `225.86 ns/op`，相对 baseline 的总 speedup 达到 `1.29x`，基本追平 `std::set`，`inline+pool/STL = 1.01`。这说明在较大规模 workload 下，节点小对象内存分配和节点的 locality 是影响 `ESet` 性能的主要因素之一。

### 3.4 普通查找与边界查询

`find`、`lower_bound`、`upper_bound` 的结果如下：

| operation | baseline/STL | inline+pool/STL | inline+pool speedup vs baseline |
|---|---:|---:|---:|
| find | 1.30 | 0.90 | 1.45 |
| lower_bound | 1.31 | 0.94 | 1.39 |
| upper_bound | 1.32 | 0.94 | 1.40 |

这些操作本身不涉及节点分配，因此 memory pool 对它们的改善不能只解释为减少 `new/delete`。更合理的解释是：chunk pool 使节点分布更集中，沿树路径访问时**缓存局部性**更好，从而降低了 pointer chasing 的实际成本。

### 3.5 插入、删除与赋值

`emplace` 在启用 memory pool 后从 `303.06 ns` 降至 `258.93 ns`，相对 inline value 的 speedup 为 `1.17x`。这说明插入路径中的节点分配成本已经比较明显。

`erase` 改善更大，`inline+pool` 相比 baseline 的 speedup 为 `1.45x`，并且已经略快于 STL。删除操作既涉及树结构调整，也涉及节点销毁和内存回收，因此它能从 memory pool 中获得较明显收益。

`assignment` 是改善最明显的操作：

| variant | assignment ns |
|---|---:|
| STL | 64011.80 |
| baseline | 174350.67 |
| inline value | 186423.82 |
| inline value + memory pool | 55282.02 |

memory pool 相对 inline value 的 speedup 为 `3.37x`，相对 baseline 的 speedup 为 `3.15x`。这是符合预期的：赋值会批量复制节点，最容易受大量小对象分配的影响。节点池减少了逐节点向系统 allocator 申请内存的成本，因此对 assignment 的改善最显著。

### 3.6 迭代器与 begin/end

`begin()` 与 `end()` 已经通过缓存最左节点和 `nil` 哨兵实现为 $O(1)$。从结果看，它们与 STL 的耗时处于同一数量级。由于单次操作只有几十纳秒，容易受测量噪声影响，因此具体数据解读意义不大

`++it` 和 `--it` 的结果也较接近 STL。其中 `--it` 在最终版本中略慢，可能与当前 iterator 仍保存额外边界信息有关。

---

## 4. Range Sweep 结果

本节继续使用同样的简写：`S0` 为 baseline，`S1` 为 inline value，`S2` 为 inline value + memory pool。

### 4.1 Raw Runtime

| operation | count | STL | S0 | S1 | S2 |
|---|---:|---:|---:|---:|---:|
| range_small | 50000 | 241.31 | 337.56 | 341.18 | 256.61 |
| range_medium | 50000 | 1758.61 | 556.05 | 666.97 | 339.42 |
| range_large | 3000 | 161769.97 | 665.18 | 795.53 | 411.03 |

### 4.2 Relative Performance

| operation    | S0/STL | S1/STL | S2/STL | S0/S1 | S1/S2 | S0/S2 |
| ------------ | -----: | -----: | -----: | ----: | ----: | ----: |
| range_small  |   1.40 |   1.41 |   1.06 |  0.99 |  1.33 |  1.32 |
| range_medium |   0.32 |   0.38 |   0.19 |  0.83 |  1.97 |  1.64 |
| range_large  |  0.004 |  0.005 |  0.003 |  0.84 |  1.94 |  1.62 |

`range_small` 中，最终版本仍略慢于 STL，`inline+pool/STL = 1.06`。这是合理的：小区间内元素数 $k$ 很小，STL 的 `distance` 几乎不需要走多少节点，而 ESet 固定需要两次 rank 查询，常数不一定占优。

随着区间增大，ESet 的优势迅速显现。`range_medium` 中，`inline+pool/STL = 0.19`；`range_large` 中，`inline+pool/STL ≈ 0.003`。这说明 `range` 的优势并不是简单常数优化，而是来自复杂度差异：

$$
\text{ESet range}: O(\log n)
$$

$$
\text{std::set baseline}: O(\log n + k)
$$

其中 $k$ 是区间内元素个数。当 $k$ 较大时，STL 需要线性遍历大量节点，而 ESet 仍然只需要两次 rank 搜索。

---

## 5. OJ 结果

本地 benchmark 之外，OJ 提交结果也支持优化方向：

| version | OJ runtime |
|---|---:|
| baseline | 8332 ms |
| inline value | 7286 ms |
| inline value + memory pool | 6488 ms |

> 提交记录为 [评测详情 · ACMOJ](https://acm.sjtu.edu.cn/OnlineJudge/code/805087/)，[评测详情 · ACMOJ](https://acm.sjtu.edu.cn/OnlineJudge/code/804823/) 与 [评测详情 · ACMOJ](https://acm.sjtu.edu.cn/OnlineJudge/code/804822/)

这侧面验证了我们的优化效果


