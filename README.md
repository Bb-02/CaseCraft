# 算法题数据生成框架 — 使用手册

## 这玩意干嘛的

写算法题需要 `.in`（输入）和 `.out`（输出）文件。这个框架让你：

- **造 `.in`**：用内置函数随机生成各种数据
- **造 `.out`**：跑一遍标准题解，自动生成输出

---

# 5 分钟快速上手

## 第 1 步：一键建题

用 `newcase` 工具生成题目目录（题面 + 生成器源码的骨架）：

```
newcase Sum          # 题目英文名 = Sum
newcase Sum 求和     # 顺便带上中文标题，写进题面.md
newcase              # 交互式：按提示输入
```

会自动生成：

```
Generators/Sum/Sum.cpp    复制自 template.cpp，g_problem_id 已填好
Generators/Sum/题面.md     题面骨架（题目描述/输入/输出/样例/数据范围）
```

> 题目英文名只能用字母、数字、下划线、连字符，它同时是目录名和数据 ID。
>
> 不用工具也行：手动复制 `template.cpp`，改 `g_problem_id = "sum"`，
> 存到 `Generators/sum/sum.cpp`，再自己建个 `题面.md`。

## 第 2 步：写题面和造数据代码

题面写在 `Generators/Sum/题面.md` 里。

在 `generate_input()` 里面写：

```cpp
void generate_input() {
    DataWriter dw; // 生成 001.in, 002.in, 003.in ...

    // 手写一组样例
    dw.next([](ostream &o) {
        o << "3 7\n";            // n=3, m=7
        o << "1 2 3\n";
    });

    // 用随机函数生成
    dw.next([](ostream &o) {
        int n = rnd->next(5, 20);      // n = 5~20 随机
        o << n << '\n';
        auto a = gen_array(n, 1, 100); // n 个 1~100 的随机数
        print_vec(o, a);               // 输出到文件
    });

    // 批量生成 10 组大数据
    for (int i = 0; i < 10; i++) {
        dw.next([&](ostream &o) {
            int n = 100000;
            o << n << '\n';
            print_vec(o, gen_array(n, 1, 1e9));
        });
    }
}
```

## 第 3 步：写题解

在 `solve()` 里面写：

```cpp
void solve(istream &in, ostream &out) {
    // in = 输入流（当成 cin 用）
    // out = 输出流（当成 cout 用）
    int n, m;
    in >> n >> m;
    long long sum = 0;
    for (int i = 0; i < n; i++) {
        long long x; in >> x;
        sum += x;
    }
    out << sum % m << '\n';
}
```

## 第 4 步：编译运行

**在项目根目录编译和运行**（生成器在子目录里通过 `../../gen_lib.h` 引用核心库，
数据统一出到根目录的 `Data/`）：

```bash
# MSVC（VS 开发者命令行）
cl /utf-8 /EHsc /std:c++17 /O2 /Fe:gen.exe Generators/Sum/Sum.cpp

# g++
g++ -std=c++17 -O2 Generators/Sum/Sum.cpp -o gen

./gen        # 生成 Data/Sum_Data/001.in, 002.in ...
./gen out    # 读取 .in，跑 solve，生成 .out
./gen 12345  # 固定随机种子（让每次生成的数据一样）
```

> 怕题解写错？写个暴力就能 `./gen duipai` 自动对拍验证，见下面的[对拍]章节。

---

# 目录结构

```
CaseCraft/
├── gen_lib.h           ← 核心库（不需要动）
├── duipai.h            ← 对拍器（配合 gen_lib.h，不需要动）
├── template.cpp        ← 模板（newcase 工具从这里复制）
├── newcase.cpp         ← 一键建题工具，编译成 newcase.exe
├── example_graph.cpp   ← 完整示例（最短路径）
├── tests/              ← duipai 自测（make test）
├── README.md           ← 本文
├── Generators/         ← 题目生成器（每题一个文件夹）
│   ├── Sum/
│   │   ├── 题面.md
│   │   └── Sum.cpp     （g_problem_id = "Sum"）
│   └── Tree/
│       ├── 题面.md
│       └── Tree.cpp
└── Data/               ← 所有数据（自动生成，不上传）
    ├── Sum_Data/       ← Sum 题的数据
    │   ├── 001.in
    │   ├── 001.out
    │   └── ...
    └── Tree_Data/
        └── ...
```

---

# 函数速查表

## 随机数引擎 — `rnd`

全局变量，直接用。

| 函数 | 说明 | 示例 |
|------|------|------|
| `rnd->next(l, r)` | 整数 ∈ [l, r]，支持**负数**、**long long** | `rnd->next(-1e18, 1e18)` |
| `rnd->next(r)` | 整数 ∈ [0, r] | `rnd->next(100)` → 0~100 |
| `rnd->next_n(n)` | 整数 ∈ [0, n) | `rnd->next_n(5)` → 0~4 |
| `rnd->next_double()` | 浮点数 ∈ [0.0, 1.0) | `rnd->next_double()` → 0.573... |
| `rnd->next_double(l, r)` | 浮点数 ∈ [l, r) | `rnd->next_double(-1.5, 3.0)` |
| `rnd->chance(p)` | 以概率 p 返回 true | `rnd->chance(0.3)` → 30% 概率 |
| `rnd->shuffle(v)` | 原地打乱 vector | `rnd->shuffle(a)` |
| `rnd->pick(v)` | 随机选一个元素 | `rnd->pick(v)` → 返回 v 中某个元素 |
| `rnd->sample(v, k)` | 不放回挑 k 个 | `rnd->sample(v, 3)` → 返回 3 个 |
| `rnd->distinct(k, l, r)` | [l,r] 中取 k 个不重复整数 | `rnd->distinct(5, 1, 100)` → 5 个不同的数 |
| `rnd->next_even(l, r)` | 随机偶数 ∈ [l, r] | `rnd->next_even(0, 100)` → 0~100 间的偶数 |
| `rnd->next_odd(l, r)` | 随机奇数 ∈ [l, r] | `rnd->next_odd(-99, 99)` → -99~99 间的奇数 |
| `rnd->next_multiple(l, r, k)` | 随机 k 的倍数 ∈ [l, r] | `rnd->next_multiple(1, 100, 7)` → 7 的倍数 |

---

## 数组生成器

| 函数 | 说明 | 示例 |
|------|------|------|
| `gen_array(n, l, r)` | n 个 ∈ [l, r] 的随机整数 | `gen_array(10, -100, 100)` |
| `gen_real_array(n, l, r)` | n 个 ∈ [l, r) 的随机浮点数 | `gen_real_array(5, 0.0, 1.0)` |
| `gen_array_same(n, val)` | n 个全是 val | `gen_array_same(100, 42)` |
| `gen_sorted_array(n, l, r)` | 非降序数组 | `gen_sorted_array(10, 1, 100)` |
| `gen_strictly_increasing(n, l, r)` | 严格递增（无重复） | `gen_strictly_increasing(10, 1, 100)` |
| `gen_array_unique(n, l, r)` | 不重复随机数组（未排序） | `gen_array_unique(10, 1, 100)` |

---

## 排列生成器

| 函数 | 说明 | 示例 |
|------|------|------|
| `gen_permutation(n)` | 1..n 随机打乱 | `gen_permutation(10)` → `[3,7,1,5,...]` |
| `gen_permutation_reverse(n)` | 完全逆序 | `gen_permutation_reverse(5)` → `[5,4,3,2,1]` |
| `gen_permutation_almost_sorted(n, k)` | 几乎有序（做 k 次随机交换） | `gen_permutation_almost_sorted(100, 3)` |
| `gen_permutation_half_shuffle(n)` | 前一半和后一半各自打乱 | `gen_permutation_half_shuffle(10)` |

---

## 字符串生成器

| 函数 | 说明 | 示例 |
|------|------|------|
| `gen_string(n)` | 长度 n 的随机小写字母串 | `gen_string(10)` → `"ahfjkqwepz"` |
| `gen_string(n, charset)` | 指定字符集 | `gen_string(5, "ACGT")` → `"GATCA"` |
| `gen_string(n, "01")` | 随机 01 串 | `gen_string(8, "01")` → `"01101001"` |
| `gen_palindrome(n)` | 长度 n 的随机回文串 | `gen_palindrome(5)` → `"abcba"` |
| `gen_string_distinct(n)` | 长度 n 的全不同小写字母 | `gen_string_distinct(5)` → `"kfxap"` |

---

## 树生成器

返回 `vector<pair<int, int>>`，每条边 `u < v`。

| 函数 | 说明 |
|------|------|
| `gen_tree_prufer(n)` | 完全随机树（均匀分布） |
| `gen_tree_star(n)` | 菊花树（1 为中心） |
| `gen_tree_chain(n)` | 链（1-2-3-...-n） |
| `gen_tree_deg_capped(n, d)` | 每个节点度数 ≤ d 的随机树 |
| `gen_tree_binary(n)` | 随机二叉树（每个节点最多 2 子） |
| `gen_parent_array(n)` | 有根树(1为根)的父节点数组，返回 p[2..n] |

使用示例：

```cpp
auto edges = gen_tree_prufer(10); // 10 个节点的随机树
print_edges(o, edges);            // 输出到文件，每行 "u v"

// 另一种常见格式：父节点数组（很多树的题用这种格式）
auto p = gen_parent_array(10);    // p[0]=节点2的父节点, p[1]=节点3的父节点, ...
for (int i = 2; i <= 10; i++)
    cout << p[i - 2] << " \n"[i == 10];
```

---

## 图生成器

返回 `vector<pair<int, int>>`，每条边 `u < v`。

| 函数 | 说明 |
|------|------|
| `gen_graph_connected(n, m)` | n 节点 m 边的连通无向图 |
| `gen_dag(n, m)` | n 节点 m 边的有向无环图 |
| `gen_graph_complete(n)` | n 节点的完全图 |
| `gen_graph_bipartite(n1, n2, m)` | 二分图：左 n1 右 n2 节点，m 条边 |
| `gen_graph_directed(n, m)` | 有向图（允许环，无重边） |

---

## 其他生成器

| 函数 | 说明 |
|------|------|
| `gen_partition(n, k)` | 把 n 随机分成 k 个正整数的和 |

---

## 输出函数

| 函数 | 说明 | 示例 |
|------|------|------|
| `print_vec(o, v)` | 输出 vector 到 ostream | `print_vec(o, a)` 空格分隔，末尾换行 |
| `print_vec(o, v, "\n")` | 换行分隔 | `print_vec(o, a, "\n")` |
| `print_vec(v)` | 输出到屏幕（调试） | `print_vec(a)` |
| `print_real_vec(o, v, p)` | 输出浮点 vector，p 位小数 | `print_real_vec(o, f, 6)` |
| `print_edges(o, e)` | 输出边集 | `print_edges(o, edges)` |
| `println(args...)` | 输出到屏幕 | `println("n =", n)` |

---

## DataWriter — 写 .in 文件

```cpp
DataWriter dw;              // 001.in, 002.in, 003.in ...
DataWriter dw("in");        // in_001.in, in_002.in ...
DataWriter dw("in", "out"); // in_001.out, in_002.out ...（自定义后缀）
```

**写数据只有一种方式：**

```cpp
dw.next([](ostream &o) {
    o << "内容\n";
    print_vec(o, gen_array(10, 1, 100));
});
```

lambda 用 `[&]` 捕获外面的变量：

```cpp
int n = rnd->next(1, 100);
dw.next([&](ostream &o) {
    o << n << '\n';
});
```

**`dw.counter`** 是已生成的文件数量。

---

## gen_output — 生成 .out 文件

```cpp
gen_output(solve);
```

自动扫描 `Data/{id}_Data/` 下所有 `.in` 文件，逐个读入 → 跑 `solve(in, out)` → 写出 `.out`。

`solve` 签名固定为：

```cpp
void solve(istream &in, ostream &out) {
    // in 当 cin 用，out 当 cout 用
}
```

---

# 对拍 — 用暴力自动验证题解正确性

solve 写完心里没底？写个暴力让它俩对拍，不一致立刻抓出来。

在题目 .cpp 里写两个函数（`newcase` 生成的骨架里自带这两个空壳）：

```cpp
// ① 造一个【小】随机数据。brute 要跑得动，规模控制在几十以内。
//    每轮自动换一个独立种子的 rnd，放心用 rnd-> 生成随机数。
void gen_case(ostream &o) {
    int n = rnd->next(1, 10);
    o << n << '\n';
    print_vec(o, gen_array(n, -100, 100));
}

// ② 正确性显然的暴力解。输出格式必须和 solve 完全一致。
void brute(istream &in, ostream &out) {
    int n; in >> n;
    long long s = 0;
    for (int i = 0; i < n; i++) { long long x; in >> x; s += x; }
    out << s << '\n';
}
```

然后一条命令：

```bash
./gen duipai              # ③ 默认 100 轮：造数据 → solve/brute 各跑一遍 → 比对
./gen duipai 10000        # 轰炸 10000 轮
./gen duipai 1000 2024    # 固定种子，可复现
```

每 100 轮打印一次进度；**输出不一致会立刻停下**（退出码 1），现场存在
`Data/{id}_Duipai/` 下：

```
005.in          出事的输入
005.solve.out   solve 的输出
005.brute.out   brute 的输出
```

目录里还会滚动保存 `cur.in`（当前这轮的输入）。对拍在进程内跑，没有超时——
万一 solve 或 brute 死循环，Ctrl+C 之后直接看 `cur.in`，就是肇事的那组数据。

**比较规则**：按行比较；忽略行尾空白和文末空行；行首空白、中间空行、行内
空格数量都参与比较（偏严格，图形输出安全）。浮点题要按误差比较的话，调
`run_duipai_core()` + `DuipaiOptions::compare` 自定义比较器（见 `duipai.h`）。

**可复现**：每轮数据只由 (总种子, 轮数) 决定。`./gen duipai 1000 2024` 在第 7 轮
出了不一致，改天重跑同一条命令，前 6 轮一字不差，第 7 轮必现。

> 想测外部编译好的 exe、或做交互题对拍？`duipai.h` 的核心是按参数驱动的
> （`DuipaiSolver` 适配器），加一个"跑子进程"的适配器就能接入，核心不用动。

---

# 常见用法集锦

## 造极端数据

```cpp
// 全是 1
dw.next([](ostream &o) {
    int n = 100000;
    o << n << '\n';
    print_vec(o, gen_array_same(n, 1));
});

// 严格递增
dw.next([](ostream &o) {
    int n = 100000;
    o << n << '\n';
    print_vec(o, gen_strictly_increasing(n, 1, 1'000'000'000));
});

// 完全逆序排列
dw.next([](ostream &o) {
    int n = 100000;
    o << n << '\n';
    print_vec(o, gen_permutation_reverse(n));
});
```

## 造带权树

```cpp
dw.next([](ostream &o) {
    int n = 100;
    o << n << '\n';
    auto edges = gen_tree_prufer(n);
    for (auto [u, v] : edges) {
        int w = rnd->next(1, 1000); // 随机边权
        o << u << ' ' << v << ' ' << w << '\n';
    }
});
```

## 造 DAG + 边权

```cpp
dw.next([](ostream &o) {
    int n = 100, m = 300;
    auto edges = gen_dag(n, m);
    o << n << ' ' << m << '\n';
    for (auto [u, v] : edges) {
        int w = rnd->next(-100, 100); // 边权可正可负
        o << u << ' ' << v << ' ' << w << '\n';
    }
});
```

## 一个文件里多组询问

```cpp
dw.next([](ostream &o) {
    int t = rnd->next(1, 10);
    o << t << '\n';
    while (t--) {
        int n = rnd->next(1, 100);
        o << n << '\n';
        print_vec(o, gen_array(n, 1, 1000));
    }
});
```

## 多个规模的数据用 TestCase 管理

```cpp
vector<TestCase> cases = {
    {"sample",  5,   10,      100},
    {"small",   20,  100,     1000},
    {"medium",  1000, 10000,  100000},
    {"large",   100000, 500000, 1'000'000'000LL},
};
for (auto &tc : cases) {
    dw.next([&](ostream &o) {
        o << tc.n << ' ' << tc.m << ' ' << tc.max_val << '\n';
        // ...
    });
}
```

## 固定种子让数据可复现

```bash
./gen 2024       # 用种子 2024 生成 → 每次跑出来的数据一模一样
./gen 2024 out   # 同样种子生成输出
```

不同题目的种子会自动混入 `g_problem_id` 的哈希值，所以即使多个题目用同一个种子号，生成的随机序列也互不干扰。不指定种子时用系统时钟自动生成。

---

# 命令行参数

| 命令 | 效果 |
|------|------|
| `./gen` | 生成输入（随机种子） |
| `./gen 12345` | 生成输入（种子=12345，可复现） |
| `./gen out` | 生成输出 |
| `./gen 12345 out` | 生成输出（指定种子，`out` 位置随意） |
| `./gen duipai` | 对拍 100 轮（solve vs brute） |
| `./gen duipai 1000` | 对拍 1000 轮 |
| `./gen duipai 1000 12345` | 对拍 1000 轮（种子=12345，可复现） |

对拍发现不一致 / 运行出错时退出码为 1，方便脚本判断。

---

# 添加新题目 checklist

1. 运行 `newcase Xxx`（或 `newcase` 按提示输入）
2. 写 `Generators/Xxx/题面.md`
3. 写 `generate_input()` — 用 `dw.next()` 和 `gen_xxx()` 造数据
4. 写 `solve(istream&, ostream&)` — 题解
5. 写 `gen_case()` 和 `brute()` — 对拍三件套，`./gen duipai` 验证题解
6. 在根目录编译：`cl /utf-8 /EHsc /std:c++17 /O2 /Fe:gen.exe Generators/Xxx/Xxx.cpp`
7. `./gen duipai` 对拍通过 → `./gen` 生成输入，`./gen out` 生成输出
8. 检查 `Data/Xxx_Data/` 目录下的 `.in` 和 `.out`
