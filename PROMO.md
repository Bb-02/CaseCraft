# 🛠 CaseCraft — 小白也能用的算法题数据生成框架

## 一句话介绍

一个 **C++ 单头文件** 的数据生成库，让你用几行代码就能造出各种算法题的测试数据。不需要学任何新工具，会写 C++ 就会用。

---

## 为什么要造这个

写算法题的时候，最头大的不是题解，是**造数据**。

手写样例累死，随机生成又要写一堆 `rand()` 和 `fopen`。每次出新题还要再写一遍。出一次校赛，造数据的时间比写题解还长。

我要的就是一个东西：**改两行就能跑，数据自动按题目分好目录，输入输出一起出。** 找不到合适的，就自己写了一个。

---

## 它长什么样

```cpp
#include "gen_lib.h"
using namespace std;

string g_problem_id = "sum";  // 题目 ID

void generate_input() {
    DataWriter dw;  // 自动生成 001.in, 002.in ...

    // 一组样例
    dw.next([](ostream &o) {
        o << "3 5\n1 2 3\n";
    });

    // 随机 10 组数据
    for (int i = 0; i < 10; i++) {
        dw.next([&](ostream &o) {
            int n = rnd->next(100, 100000);     // 随机 n
            o << n << '\n';
            print_vec(o, gen_array(n, 1, 1e9)); // 随机数组
        });
    }
}

void solve(istream &in, ostream &out) {
    // 你的题解，in 当 cin 用，out 当 cout 用
    int n, m; in >> n >> m;
    // ...
    out << ans << '\n';
}
```

然后：

```bash
./gen        # 生成 Data/sum_Data/001.in ~ 011.in
./gen out    # 自动读入 .in，跑 solve，生成 .out
./gen duipai # 写好暴力后一键对拍，验证 solve 的正确性
./gen bench  # 横向计时，测算法改动/剪枝带来多少提升
```

完事。

---

## 已经帮你写好的

| 你想要什么 | 一行代码 |
|-----------|----------|
| 随机数组 | `gen_array(n, l, r)` |
| 随机排列 | `gen_permutation(n)` |
| 逆序排列 | `gen_permutation_reverse(n)` |
| 几乎有序 | `gen_permutation_almost_sorted(n, k)` |
| 随机字符串 | `gen_string(n)` |
| 回文串 | `gen_palindrome(n)` |
| 随机树 | `gen_tree_prufer(n)` |
| 菊花树 | `gen_tree_star(n)` |
| 二叉树 | `gen_tree_binary(n)` |
| 连通图 | `gen_graph_connected(n, m)` |
| 有向无环图 | `gen_dag(n, m)` |
| 完全图 | `gen_graph_complete(n)` |
| 二分图 | `gen_graph_bipartite(n1, n2, m)` |
| **对拍验证题解** | `./gen duipai`，solve vs 暴力，不一致自动留现场 |
| **计时横向测评** | `./gen bench`，同一输入多实现对比，min/median + 加速比 |

## 对比

| 手工造数据 | CaseCraft |
|-----------|--------|
| 手动 fopen / freopen | `DataWriter` 自动搞定 |
| 每次 rand() 自己写范围 | `rnd->next(l, r)` 支持负数、long long、浮点 |
| 文件乱放 | 自动按题目分目录 `Data/{id}_Data/` |
| 写脚本跑标程 | `./gen out` 一键生成 .out |
| 换一题重写一遍 | 改 `g_problem_id`，复制模板 |

---

## 适合谁用

- **自己做题** — 没现成数据，想自己测边界情况
- **出校内比赛** — 批量造 N 组不同规模的数据，生成答案
- **出 OJ 题目** — 按 TEST 格式输出，直接上传
- **教同学写题** — 给学生配套数据和答案

---

## 怎么开始

```bash
git clone https://github.com/ChengMaoMao/CaseCraft.git
cp template.cpp my_problem.cpp
# 改 g_problem_id → 写 generate_input() → 写 solve() → 编译运行
```

README 里有完整的手把手教程。

---

## 轻量

- **单头文件** — 只 include `gen_lib.h`，不需要链接任何库
- **标准 C++17** — MSVC / g++ / clang 都能编译
- **不到 500 行** — 代码干净，想看源码也看得完

---

## 最后

这个东西是我自己写题的时候顺手做的，觉得好用就放出来了。如果你也觉得造数据麻烦，试试这个。

有问题可以提 Issue，觉得不错给个 ⭐，就酱。
