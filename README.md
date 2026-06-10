# 算法题数据生成框架 — 使用手册

## 这玩意干嘛的

写算法题需要 `.in`（输入）和 `.out`（输出）文件。这个框架让你：

- **造 `.in`**：用内置函数随机生成各种数据
- **造 `.out`**：跑一遍标准题解，自动生成输出
- **对拍检查**：自动把 `.in` 输入到你的程序，并和 `.out` 比对

---

# 5 分钟快速上手

## 第 1 步：复制模板

复制 `template.cpp` → 重命名为你的题目的名字（比如 `sum.cpp`）
```
cp template.cpp sum.cpp
```

## 第 2 步：改题目 ID

打开文件，找到第一行能改的代码：

```cpp
string g_problem_id = "sum"; // 改这里
```

**这个 ID 决定数据存到哪。** 改成 `"sum"`，数据就去 `Data/sum_Data/`。

## 第 3 步：写造数据代码

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

## 第 4 步：写题解

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

## 第 5 步：编译运行

```bash
g++ -std=c++17 -O2 sum.cpp -o gen

./gen        # 生成 Data/sum_Data/001.in, 002.in ...
./gen out    # 读取 .in，跑 solve，生成 .out
./gen 12345  # 固定随机种子（让每次生成的数据一样）
```

## 一键对拍 / 检查自己的程序

假设：

- 生成器文件是 `sum.cpp`
- 待测程序是 `main.cpp`
- `sum.cpp` 里的 `solve(istream&, ostream&)` 是标准题解

先编译：

```bash
g++ -std=c++17 -O2 sum.cpp -o gen
g++ -std=c++17 -O2 main.cpp -o my_program
```

一键完成“造输入 → 造标准输出 → 跑待测程序 → 比对输出”：

```bash
./gen duipai ./my_program 12345
```

也可以拆开跑：

```bash
./gen 12345
./gen out
./gen check ./my_program
```

默认按 token 比较，忽略空格、换行差异。

如果需要严格逐字节比较：

```bash
./gen check ./my_program --strict
```

检查结果含义：

- `[AC]`：该测试点通过
- `[WA]`：输出不一致
- `[RE]`：待测程序运行失败或返回非 0
- `[MISS]`：找不到对应的 `.out` 标准输出文件

如果某个测试点错误，会保留这些文件方便调试：

```text
001.user.out  # 待测程序输出
001.user.err  # 待测程序 stderr
```

命令返回值：所有测试点通过时返回 `0`，只要有失败就返回非 `0`，所以可以接入 CI。

---

# 目录结构

```
CaseCraft/
├── gen_lib.h           ← 核心库（不需要动）
├── template.cpp        ← 模板，复制这个开始写新题
├── example_graph.cpp   ← 完整示例（最短路径）
├── README.md           ← 本文
└── Data/               ← 所有数据
    ├── sum_Data/       ← sum 题的数据
    │   ├── 001.in
    │   ├── 001.out
    │   ├── 002.in
    │   └── 002.out
    └── tree_Data/      ← tree 题的数据
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

---

## 数组生成器

| 函数 | 说明 | 示例 |
|------|------|------|
| `gen_array(n, l, r)` | n 个 ∈ [l, r] 的随机整数 | `gen_array(10, -100, 100)` |
| `gen_real_array(n, l, r)` | n 个 ∈ [l, r) 的随机浮点数 | `gen_real_array(5, 0.0, 1.0)` |
| `gen_array_same(n, val)` | n 个全是 val | `gen_array_same(100, 42)` |
| `gen_sorted_array(n, l, r)` | 非降序数组 | `gen_sorted_array(10, 1, 100)` |
| `gen_strictly_increasing(n, l, r)` | 严格递增（无重复） | `gen_strictly_increasing(10, 1, 100)` |

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

使用示例：

```cpp
auto edges = gen_tree_prufer(10); // 10 个节点的随机树
print_edges(o, edges);            // 输出到文件，每行 "u v"
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

## check_outputs — 对拍检查

```cpp
check_outputs("./my_program");           // token 比较
check_outputs("./my_program", "", true); // 严格逐字节比较
```

通常不需要直接调用这个函数，直接使用命令行即可：

```bash
./gen check ./my_program
./gen duipai ./my_program 12345
```

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

---

# 命令行参数

| 命令 | 效果 |
|------|------|
| `./gen` | 生成输入（随机种子） |
| `./gen 12345` | 生成输入（种子=12345，可复现） |
| `./gen out` | 生成输出 |
| `./gen 12345 out` | 生成输出（指定种子） |

---

# 添加新题目 checklist

1. `cp template.cpp xxx.cpp`
2. 修改 `g_problem_id = "xxx"`
3. 写 `generate_input()` — 用 `dw.next()` 和 `gen_xxx()` 造数据
4. 写 `solve(istream&, ostream&)` — 题解
5. `g++ -std=c++17 -O2 xxx.cpp -o gen`
6. `./gen` 生成输入，`./gen out` 生成输出
7. 检查 `Data/xxx_Data/` 目录下的 `.in` 和 `.out`
