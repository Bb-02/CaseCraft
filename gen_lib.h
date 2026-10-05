#pragma once
/**
 * gen_lib.h — 算法题数据生成核心库
 *
 * 用法：
 *   1. 运行 newcase 你的题目名（生成 Generators/{题目名}/ 目录和骨架）
 *   2. 编写 generate_input() 和 solve()
 *   3. 在项目根目录编译运行：
 *        g++ -std=c++17 -O2 Generators/{题目名}/{题目名}.cpp -o gen
 *        ./gen        生成输入 (.in)
 *        ./gen out    生成输出 (.out)
 *
 * 生成的数据自动放入 Data/{problem_id}_Data/ 目录。
 */

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>
using namespace std;
namespace fs = std::filesystem;

// ============================================================
// 全局配置 — 修改这里来切换题目
// ============================================================
extern string g_problem_id; // 在 .cpp 文件中定义：string g_problem_id = "xxx";

// ============================================================
// 随机数引擎
// ============================================================
struct Random {
    mt19937_64 engine;

    Random() {
        auto now = chrono::steady_clock::now().time_since_epoch().count();
        engine.seed(now);
    }

    explicit Random(uint64_t seed) { engine.seed(seed); }

    // ========== 整数随机 ==========

    // [l, r] 闭区间整数（支持负数，支持 long long 范围）
    long long next(long long l, long long r) {
        assert(l <= r);
        return uniform_int_distribution<long long>(l, r)(engine);
    }

    // [0, r]
    long long next(long long r) { return next(0LL, r); }

    // [0, n)
    long long next_n(long long n) {
        assert(n > 0);
        return next(0LL, n - 1);
    }

    // [l, r] 范围内随机偶数
    long long next_even(long long l, long long r) {
        if (l % 2 != 0) l++;
        if (r % 2 != 0) r--;
        assert(l <= r);
        return l + next(0LL, (r - l) / 2) * 2;
    }

    // [l, r] 范围内随机奇数
    long long next_odd(long long l, long long r) {
        if (l % 2 == 0) l++;
        if (r % 2 == 0) r--;
        assert(l <= r);
        return l + next(0LL, (r - l) / 2) * 2;
    }

    // [l, r] 范围内随机 k 的整数倍（支持负数和 long long）
    long long next_multiple(long long l, long long r, long long k) {
        assert(k > 0);
        auto rem = [&](long long x) { return ((x % k) + k) % k; };
        long long lo = l + (k - rem(l)) % k;
        long long hi = r - rem(r);
        assert(lo <= hi);
        return lo + next(0LL, (hi - lo) / k) * k;
    }

    // ========== 浮点数随机 ==========

    // [0.0, 1.0) 随机浮点数
    double next_double() {
        return uniform_real_distribution<double>(0.0, 1.0)(engine);
    }

    // [l, r) 随机浮点数
    double next_double(double l, double r) {
        return uniform_real_distribution<double>(l, r)(engine);
    }

    // 兼容旧名
    double next_real() { return next_double(); }
    double next_real(double l, double r) { return next_double(l, r); }

    // 以概率 p 返回 true
    bool chance(double p) { return next_real() < p; }

    // Fisher-Yates shuffle
    template <typename T> void shuffle(vector<T> &v) {
        std::shuffle(v.begin(), v.end(), engine);
    }

    // 随机挑选一个元素（vector 版本）
    template <typename T> const T &pick(const vector<T> &v) {
        assert(!v.empty());
        return v[next_n(v.size())];
    }

    // 随机挑选一个字符（string 版本）
    char pick(const string &s) {
        assert(!s.empty());
        return s[next_n(s.size())];
    }

    // 不放回地挑选 k 个元素（Floyd 抽样，O(k)，与 v.size() 无关）
    template <typename T> vector<T> sample(const vector<T> &v, int k) {
        assert(0 <= k && k <= (int)v.size());
        vector<T> res;
        res.reserve(k);
        for (long long i : distinct(k, 0, (long long)v.size() - 1))
            res.push_back(v[(size_t)i]);
        return res;
    }

    // 生成唯一的随机整数集合（[l, r] 中取 k 个不重复的）
    // Floyd 抽样：O(k) 时间 / O(k) 空间，与 r-l 大小无关——
    // 旧版在 k 接近区间总量时要把整个区间物化出来（1e9 范围 = 8GB），现在不会了
    vector<long long> distinct(int k, long long l, long long r) {
        vector<long long> res;
        if (k <= 0) return res; // k=0 直接返回（gen_partition(1,1) 的空区间会走到）
        // 区间长度用无符号减法：l=-2^62、r=2^62 时 r-l+1 会溢出有符号数
        assert((unsigned long long)(k - 1) <= (unsigned long long)r - (unsigned long long)l);
        res.reserve(k);
        // swap-map：记录"某个位置当前代表的值"（没映射的位置代表它自己）。
        // 每轮从 [l, r-i] 抽一个位置，取出它现值输出，再把"末尾位置"的值挪进去，
        // 等价于对概念数组做 partial Fisher-Yates。
        unordered_map<long long, long long> slot;
        for (long long i = 0; i < k; i++) {
            long long t = next(l, r - i);
            auto it = slot.find(t);
            long long val = it != slot.end() ? it->second : t;
            res.push_back(val);
            auto it2 = slot.find(r - i);
            slot[t] = it2 != slot.end() ? it2->second : r - i;
        }
        return res;
    }
};

// 全局随机数实例（在 main 中初始化）
Random *rnd = nullptr;

// ============================================================
// 基础生成器
// ============================================================

// 生成 [l, r] 范围内的 n 个随机整数
vector<long long> gen_array(int n, long long l, long long r) {
    vector<long long> a(n);
    for (int i = 0; i < n; i++) a[i] = rnd->next(l, r);
    return a;
}

// 将 n 随机分成 k 个正整数，返回每份大小（和为 n）
vector<int> gen_partition(int n, int k) {
    assert(n >= k && k >= 1);
    auto cuts = rnd->distinct(k - 1, 1, n - 1);
    sort(cuts.begin(), cuts.end());

    vector<int> res(k);
    int pre = 0;
    for (int i = 0; i < k - 1; i++) {
        res[i] = cuts[i] - pre;
        pre = cuts[i];
    }
    res[k - 1] = n - pre;
    return res;
}

// 生成非降序数组
vector<long long> gen_sorted_array(int n, long long l, long long r) {
    auto a = gen_array(n, l, r);
    sort(a.begin(), a.end());
    return a;
}

// 生成 n 个 [l, r) 的随机浮点数
vector<double> gen_real_array(int n, double l, double r) {
    vector<double> a(n);
    for (int i = 0; i < n; i++) a[i] = rnd->next_double(l, r);
    return a;
}

// 生成全是同一个值的数组
vector<long long> gen_array_same(int n, long long val) {
    return vector<long long>(n, val);
}

// 生成严格递增（不重复）的数组：[l, r] 中取 n 个不同的数，排序
vector<long long> gen_strictly_increasing(int n, long long l, long long r) {
    assert(n <= r - l + 1);
    auto vals = rnd->distinct(n, l, r);
    sort(vals.begin(), vals.end());
    return vals;
}

// 生成不重复的随机数组（保持随机顺序，未排序）
vector<long long> gen_array_unique(int n, long long l, long long r) {
    assert(n <= r - l + 1);
    return rnd->distinct(n, l, r);
}

// 生成 n 个区间 [L, R]：l <= L <= R <= r；strict = true 时要求 L < R
vector<pair<long long, long long>> gen_intervals(int n, long long l, long long r,
                                                 bool strict = false) {
    assert(strict ? r > l : r >= l);
    vector<pair<long long, long long>> res((size_t)n);
    for (auto &iv : res) {
        long long L = rnd->next(l, strict ? r - 1 : r);
        long long R = strict ? rnd->next(L + 1, r) : rnd->next(L, r);
        iv = {L, R};
    }
    return res;
}

// 生成 n 个互不相同的二维整点 (x, y)：x ∈ [x_lo, x_hi]，y ∈ [y_lo, y_hi]
// 注意：两个方向的跨度都不要超过 3e9（去重 key 要打包进 64 位）；
// n 接近点阵总量时会退化成拒绝采样，请留给足够稀疏的范围
vector<pair<long long, long long>> gen_points_2d(int n, long long x_lo, long long x_hi,
                                                 long long y_lo, long long y_hi) {
    assert(n >= 0);
    long long y_span = y_hi - y_lo + 1;
    // 跨度上限保证下面乘积（<= 9e18）不溢出 long long
    assert(x_hi - x_lo + 1 <= 3'000'000'000LL && y_span <= 3'000'000'000LL);
    assert((x_hi - x_lo + 1) * y_span >= n);
    unordered_set<long long> used;
    used.reserve((size_t)n * 2);
    vector<pair<long long, long long>> res;
    res.reserve((size_t)n);
    while ((int)res.size() < n) {
        long long x = rnd->next(x_lo, x_hi), y = rnd->next(y_lo, y_hi);
        long long key = (x - x_lo) * y_span + (y - y_lo);
        if (used.insert(key).second) res.push_back({x, y});
    }
    return res;
}

// ============================================================
// 排列生成器
// ============================================================

// 1..n 的随机排列
vector<int> gen_permutation(int n) {
    vector<int> p(n);
    iota(p.begin(), p.end(), 1);
    rnd->shuffle(p);
    return p;
}

// 逆序排列：n, n-1, ..., 2, 1
vector<int> gen_permutation_reverse(int n) {
    vector<int> p(n);
    for (int i = 0; i < n; i++) p[i] = n - i;
    return p;
}

// 几乎有序的排列：先生成 1..n，再随机做 k 次交换
vector<int> gen_permutation_almost_sorted(int n, int k) {
    vector<int> p(n);
    iota(p.begin(), p.end(), 1);
    for (int t = 0; t < k; t++) {
        int i = rnd->next_n(n), j = rnd->next_n(n);
        swap(p[i], p[j]);
    }
    return p;
}

// 前一半/后一半 分开 shuffle 的排列
vector<int> gen_permutation_half_shuffle(int n) {
    vector<int> p(n);
    iota(p.begin(), p.end(), 1);
    int mid = n / 2;
    std::shuffle(p.begin(), p.begin() + mid, rnd->engine);
    std::shuffle(p.begin() + mid, p.end(), rnd->engine);
    return p;
}

// ============================================================
// 字符串生成器
// ============================================================

// 生成长度为 n 的随机小写字母串
string gen_string(int n) {
    string s(n, ' ');
    for (int i = 0; i < n; i++) s[i] = 'a' + rnd->next_n(26);
    return s;
}

// 生成长度为 n、字符集为 charset 的随机串
string gen_string(int n, const string &charset) {
    assert(!charset.empty());
    string s(n, ' ');
    for (int i = 0; i < n; i++) s[i] = rnd->pick(charset);
    return s;
}

// 生成随机回文串
string gen_palindrome(int n) {
    string half = gen_string((n + 1) / 2);
    string s = half;
    for (int i = n / 2 - 1; i >= 0; i--) s += half[i];
    return s;
}

// 生成长度为 n 的全部不同的随机小写字母串（n <= 26）
string gen_string_distinct(int n) {
    assert(n <= 26);
    string chars = "abcdefghijklmnopqrstuvwxyz";
    auto picked = rnd->sample(vector<char>(chars.begin(), chars.end()), n);
    return string(picked.begin(), picked.end());
}

// 生成周期为 period 的随机串（长度 n，最后一段可能截断）。
// 周期串有长 border，常用来卡 KMP / 哈希
string gen_string_period(int n, int period) {
    assert(n >= 0 && period >= 1);
    string block = gen_string(period);
    string s;
    s.reserve((size_t)n);
    for (int i = 0; i < n; i++) s += block[i % period];
    return s;
}

// 生成长度为 n 的合法括号串（n 为偶数）：任意前缀 ')' 不多于 '('，整体平衡
string gen_bracket(int n) {
    assert(n >= 0 && n % 2 == 0);
    string s;
    s.reserve((size_t)n);
    int bal = 0;
    for (int i = 0; i < n; i++) {
        int open_used = (i + bal) / 2; // 已放的开括号数
        bool open;
        if (bal == 0)
            open = true; // 必须开
        else if (open_used == n / 2)
            open = false; // 开括号用完，必须关
        else
            open = rnd->chance(0.5);
        s += open ? '(' : ')';
        bal += open ? 1 : -1;
    }
    return s;
}

// ============================================================
// 树生成器
// ============================================================

// n 个节点的随机树（prufer 序列法），节点编号 1..n
vector<pair<int, int>> gen_tree_prufer(int n) {
    if (n == 1) return {};
    if (n == 2) return {{1, 2}};
    vector<int> prufer(n - 2);
    for (int i = 0; i < n - 2; i++) prufer[i] = rnd->next(1, n);
    vector<int> deg(n + 1, 1);
    for (int x : prufer) deg[x]++;
    set<int> leaves;
    for (int v = 1; v <= n; v++)
        if (deg[v] == 1) leaves.insert(v);
    vector<pair<int, int>> edges;
    for (int x : prufer) {
        int leaf = *leaves.begin();
        leaves.erase(leaves.begin());
        edges.push_back({min(leaf, x), max(leaf, x)});
        if (--deg[x] == 1) leaves.insert(x);
    }
    int u = *leaves.begin(), v = *next(leaves.begin());
    edges.push_back({min(u, v), max(u, v)});
    return edges;
}

// n 个节点的菊花树（1 为中心）
vector<pair<int, int>> gen_tree_star(int n) {
    vector<pair<int, int>> edges;
    for (int i = 2; i <= n; i++) edges.push_back({1, i});
    return edges;
}

// n 个节点的链
vector<pair<int, int>> gen_tree_chain(int n) {
    vector<pair<int, int>> edges;
    for (int i = 1; i < n; i++) edges.push_back({i, i + 1});
    return edges;
}

// n 个节点、每个节点度数不超过 max_deg 的随机树
vector<pair<int, int>> gen_tree_deg_capped(int n, int max_deg) {
    assert(max_deg >= 2);
    vector<int> deg(n + 1);
    vector<pair<int, int>> edges;
    vector<int> cand; // 度数还没满的节点，可作为后续节点的父节点
    cand.push_back(1);
    for (int v = 2; v <= n; v++) {
        int i = (int)rnd->next_n(cand.size());
        int u = cand[i];
        edges.push_back({min(u, v), max(u, v)});
        deg[u]++, deg[v]++;
        if (deg[u] >= max_deg) {
            cand[i] = cand.back(); // 与末尾交换后删除，避免移动整个尾部
            cand.pop_back();
        }
        cand.push_back(v);
    }
    return edges;
}

// n 个节点的随机二叉树（每个节点最多 2 个子节点，1 为根）
vector<pair<int, int>> gen_tree_binary(int n) {
    vector<int> children(n + 1, 0);
    vector<pair<int, int>> edges;
    vector<int> cand; // 孩子数还没满 2 的节点，可作为后续节点的父节点
    cand.push_back(1);
    for (int v = 2; v <= n; v++) {
        int i = (int)rnd->next_n(cand.size());
        int u = cand[i];
        edges.push_back({min(u, v), max(u, v)});
        if (++children[u] == 2) {
            cand[i] = cand.back(); // 与末尾交换后删除，避免移动整个尾部
            cand.pop_back();
        }
        cand.push_back(v);
    }
    return edges;
}

// n 个节点以 1 为根的有根树，返回父节点数组 p[2..n]（p[1]=0 略去）
vector<int> gen_parent_array(int n) {
    if (n <= 1) return {};
    auto edges = gen_tree_prufer(n);
    vector<vector<int>> adj(n + 1);
    for (auto [u, v] : edges) {
        adj[u].push_back(v);
        adj[v].push_back(u);
    }
    vector<int> parent(n + 1);
    queue<int> q;
    vector<bool> vis(n + 1);
    q.push(1);
    vis[1] = true;
    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (int v : adj[u])
            if (!vis[v]) vis[v] = true, parent[v] = u, q.push(v);
    }
    return vector<int>(parent.begin() + 2, parent.end());
}

// n 个节点的毛毛虫树：一条主干链，其余节点轮流挂到主干的叶子。
// 度数结构对树上算法（长链剖分、点分治的退化情形）很有杀伤力
vector<pair<int, int>> gen_tree_caterpillar(int n) {
    vector<pair<int, int>> edges;
    if (n <= 1) return edges;
    int spine = max(1, n / 2); // 主干节点数
    for (int i = 1; i < spine; i++) edges.push_back({i, i + 1});
    for (int v = spine + 1; v <= n; v++) {
        int u = (v - spine - 1) % spine + 1; // 轮转，保证叶子够多时每个主干节点都有叶子
        edges.push_back({min(u, v), max(u, v)});
    }
    return edges;
}

// ============================================================
// 图生成器
// ============================================================

// ============================================================
// 内部工具（genlib_detail）—— 图生成器用，一般不直接调用
// ============================================================
namespace genlib_detail {

// 稠密分支候选空间上限：补集路径要全扫一遍候选（O(total)），
// 超过上限退回稀疏拒绝采样（那种规模的稠密图意味着 GB 级输入文件，现实遇不到）
constexpr long long DENSE_SCAN_CAP = 400'000'000LL;

// 反复过采样 + 排序去重 + 部分洗牌，取出 k 个互不相同的候选。
// draw() 每次返回一条候选（调用方负责过滤结构性非法的，如自环/树边）。
// 首轮过采样 1.5 倍，不够再缩小批量补抽；候选空间耗尽仍不足则抛异常。
// 结果是候选空间中均匀随机的 k-子集（候选可交换 ⇒ 无偏）。
template <typename T, typename Draw>
vector<T> draw_distinct(long long k, Draw draw) {
    vector<T> pool;
    if (k <= 0) return pool;
    long long batch = k + k / 2 + 64;
    while (true) {
        size_t before = pool.size();
        for (long long i = 0; i < batch; i++) pool.push_back(draw());
        sort(pool.begin(), pool.end());
        pool.erase(unique(pool.begin(), pool.end()), pool.end());
        if ((long long)pool.size() >= k) break;
        if (pool.size() == before)
            throw runtime_error("draw_distinct: 候选空间已耗尽（k 超过可用的不同候选数）");
        batch = max<long long>(64, batch / 2);
    }
    // 部分洗牌：前 k 个是池子的均匀随机 k-子集
    for (long long i = 0; i < k; i++)
        swap(pool[i], pool[i + rnd->next_n((long long)pool.size() - i)]);
    pool.resize((size_t)k);
    return pool;
}

// ---- 候选空间的双向编号 ----
// 上三角编号：无向图 / 拓扑序候选 (u, v)，0-based，u < v ∈ [0, n)，共 C(n,2) 个
inline long long tri_index(long long u, long long v, long long n) {
    return u * (n - 1) - u * (u - 1) / 2 + (v - u - 1);
}
inline pair<long long, long long> tri_uv(long long idx, long long n) {
    auto cum = [&](long long i) { return i * (n - 1) - i * (i - 1) / 2; };
    long long u = (long long)(((2.0 * n - 1) - sqrt((2.0 * n - 1) * (2.0 * n - 1) - 8.0 * idx)) / 2);
    while (u + 1 < n && cum(u + 1) <= idx) u++;
    while (u > 0 && cum(u) > idx) u--;
    return {u, u + 1 + (idx - cum(u))};
}
// 有向完全图编号：(u, v)，0-based，u ≠ v ∈ [0, n)，共 n(n-1) 个
inline long long dir_index(long long u, long long v, long long n) {
    return u * (n - 1) + (v < u ? v : v - 1);
}
inline pair<long long, long long> dir_uv(long long idx, long long n) {
    long long u = idx / (n - 1), r = idx % (n - 1);
    return {u, r < u ? r : r + 1};
}

// 稠密补集输出：候选空间 [0, total) 按序全扫，跳过缺席下标（升序），
// 把其余候选经 map_idx 映射成边输出，恰好 want 条。
template <typename MapIdx>
vector<pair<int, int>> dense_emit(long long total, long long want,
                                  const vector<long long> &absent_sorted,
                                  MapIdx map_idx) {
    vector<pair<int, int>> edges;
    edges.reserve((size_t)want);
    for (long long idx = 0, p = 0; idx < total && (long long)edges.size() < want; idx++) {
        while (p < (long long)absent_sorted.size() && absent_sorted[p] < idx) p++;
        if (p < (long long)absent_sorted.size() && absent_sorted[p] == idx) {
            p++;
            continue;
        }
        edges.push_back(map_idx(idx));
    }
    assert((long long)edges.size() == want);
    return edges;
}

} // namespace genlib_detail

// n 个节点 m 条边的简单无向连通图（n-1 <= m <= n*(n-1)/2）
vector<pair<int, int>> gen_graph_connected(int n, int m) {
    long long max_m = (long long)n * (n - 1) / 2;
    assert(m >= n - 1 && m <= max_m);

    // 先生成树保证连通
    auto edges = gen_tree_prufer(n);
    long long extra = m - (long long)(n - 1); // 还需要的非树边数
    if (extra <= 0) {
        rnd->shuffle(edges);
        return edges;
    }

    long long avail = max_m - (long long)(n - 1); // 非树候选边总数
    // 树邻接表（每个点单独排序）：判定一条边是不是树边，均摊 O(log deg)
    vector<vector<int>> tree_adj(n + 1);
    for (auto [u, v] : edges) {
        tree_adj[u].push_back(v);
        tree_adj[v].push_back(u);
    }
    for (auto &adj : tree_adj) sort(adj.begin(), adj.end());
    auto is_tree = [&](int u, int v) {
        if (tree_adj[u].size() > tree_adj[v].size()) swap(u, v);
        return binary_search(tree_adj[u].begin(), tree_adj[u].end(), v);
    };

    // ---- 稠密：改为采样"缺席"的非树边（max_m - m 条），全扫候选补全 ----
    // 旧版稠密分支要求 n <= 5000，更大的 n 会掉进拒绝采样：饱和度 99% 时
    // 期望尝试次数爆炸。补集采样对任意 n 都是 O(max_m - m) 抽样 + O(max_m) 扫描。
    if (extra > avail / 2 && max_m <= genlib_detail::DENSE_SCAN_CAP) {
        auto absent = genlib_detail::draw_distinct<long long>(max_m - m, [&] {
            int u, v;
            do {
                u = (int)rnd->next(1, n);
                v = (int)rnd->next(1, n);
            } while (u == v || is_tree(u, v));
            return genlib_detail::tri_index(min(u, v) - 1, max(u, v) - 1, n);
        });
        sort(absent.begin(), absent.end());

        // 全扫时跳过：缺席边 ∪ 树边（树边始终在场，不能重复输出）
        vector<long long> skip;
        skip.reserve(absent.size() + edges.size());
        for (auto [u, v] : edges)
            skip.push_back(genlib_detail::tri_index(u - 1, v - 1, n));
        skip.insert(skip.end(), absent.begin(), absent.end());
        sort(skip.begin(), skip.end());

        auto tail = genlib_detail::dense_emit(max_m, extra, skip, [&](long long idx) {
            auto [u, v] = genlib_detail::tri_uv(idx, n);
            return make_pair((int)u + 1, (int)v + 1);
        });
        edges.insert(edges.end(), tail.begin(), tail.end());
        rnd->shuffle(edges);
        return edges;
    }

    // ---- 稀疏：过采样 + 排序去重（比逐条 set 去重快一个量级）----
    auto pool = genlib_detail::draw_distinct<pair<int, int>>(extra, [&] {
        int u, v;
        do {
            u = (int)rnd->next(1, n);
            v = (int)rnd->next(1, n);
        } while (u == v || is_tree(u, v));
        return make_pair(min(u, v), max(u, v));
    });
    edges.insert(edges.end(), pool.begin(), pool.end());
    rnd->shuffle(edges);
    return edges;
}

// n 个节点 m 条边的有向无环图
vector<pair<int, int>> gen_dag(int n, int m) {
    long long max_m = (long long)n * (n - 1) / 2;
    assert(m <= max_m);
    auto perm = gen_permutation(n);

    // 稠密：采样"缺席"的拓扑边，全扫补全（对任意 n 都是 O(max_m-m) 抽样 + O(max_m) 扫描）
    if (m > max_m / 2 && max_m <= genlib_detail::DENSE_SCAN_CAP) {
        auto absent = genlib_detail::draw_distinct<long long>(max_m - m, [&] {
            int i, j;
            do {
                i = (int)rnd->next_n(n);
                j = (int)rnd->next_n(n);
            } while (i >= j);
            return genlib_detail::tri_index(i, j, n);
        });
        sort(absent.begin(), absent.end());

        auto edges = genlib_detail::dense_emit(max_m, m, absent, [&](long long idx) {
            auto [i, j] = genlib_detail::tri_uv(idx, n);
            return make_pair(perm[(size_t)i], perm[(size_t)j]);
        });
        rnd->shuffle(edges);
        return edges;
    }

    // 稀疏：过采样 + 排序去重
    auto pool = genlib_detail::draw_distinct<pair<int, int>>(m, [&] {
        int i, j;
        do {
            i = (int)rnd->next_n(n);
            j = (int)rnd->next_n(n);
        } while (i >= j);
        return make_pair(perm[(size_t)i], perm[(size_t)j]);
    });
    return pool;
}

// n 个节点 m 条边的随机有向图（允许环，无重边无自环）
vector<pair<int, int>> gen_graph_directed(int n, int m) {
    long long max_m = (long long)n * (n - 1);
    assert(m <= max_m);

    // 稠密：采样"缺席"的有序点对，全扫补全
    if (m > max_m / 2 && max_m <= genlib_detail::DENSE_SCAN_CAP) {
        auto absent = genlib_detail::draw_distinct<long long>(max_m - m, [&] {
            int u, v;
            do {
                u = (int)rnd->next(1, n);
                v = (int)rnd->next(1, n);
            } while (u == v);
            return genlib_detail::dir_index(u - 1, v - 1, n);
        });
        sort(absent.begin(), absent.end());

        auto edges = genlib_detail::dense_emit(max_m, m, absent, [&](long long idx) {
            auto [u, v] = genlib_detail::dir_uv(idx, n);
            return make_pair((int)u + 1, (int)v + 1);
        });
        rnd->shuffle(edges);
        return edges;
    }

    // 稀疏：过采样 + 排序去重
    auto pool = genlib_detail::draw_distinct<pair<int, int>>(m, [&] {
        int u, v;
        do {
            u = (int)rnd->next(1, n);
            v = (int)rnd->next(1, n);
        } while (u == v);
        return make_pair(u, v);
    });
    return pool;
}

// n 个节点的完全图
vector<pair<int, int>> gen_graph_complete(int n) {
    vector<pair<int, int>> edges;
    for (int i = 1; i <= n; i++)
        for (int j = i + 1; j <= n; j++)
            edges.push_back({i, j});
    return edges;
}

// 二分图：左部 n1 个节点(1..n1)，右部 n2 个节点(n1+1..n1+n2)，随机 m 条边
vector<pair<int, int>> gen_graph_bipartite(int n1, int n2, int m) {
    long long max_m = (long long)n1 * n2;
    assert(m <= max_m);

    // 稠密：采样"缺席"的左右配对，全扫补全
    // （旧方案上限 max_m <= 25M，更大的稠密二分图会掉进拒绝采样死区）
    if (m > max_m / 2 && max_m <= genlib_detail::DENSE_SCAN_CAP) {
        auto absent = genlib_detail::draw_distinct<long long>(
            max_m - m, [&] { return rnd->next(0, max_m - 1); });
        sort(absent.begin(), absent.end());

        auto edges = genlib_detail::dense_emit(max_m, m, absent, [&](long long idx) {
            return make_pair((int)(idx / n2) + 1, (int)(n1 + 1 + idx % n2));
        });
        rnd->shuffle(edges);
        return edges;
    }

    // 稀疏：过采样 + 排序去重
    auto pool = genlib_detail::draw_distinct<pair<int, int>>(m, [&] {
        return make_pair((int)rnd->next(1, n1), (int)rnd->next(n1 + 1, n1 + n2));
    });
    return pool;
}

// ============================================================
// 输出工具
// ============================================================

// --- println / print_vec：同时支持 stdout 和文件流 ---
// 用法：println(o, a, b, c); 或 println(a, b, c);（默认输出到 cout）

inline void println_impl(ostream &o) { o << '\n'; }

template <typename T, typename... Args>
void println_impl(ostream &o, const T &first, const Args &...args) {
    o << first;
    if constexpr (sizeof...(args) > 0) o << ' ';
    println_impl(o, args...);
}

// 带 ostream 版本
template <typename T, typename... Args>
void println(ostream &o, const T &first, const Args &...args) {
    o << first;
    if constexpr (sizeof...(args) > 0) o << ' ';
    println_impl(o, args...);
}

// 默认输出到 cout
template <typename T, typename... Args>
void println(const T &first, const Args &...args) {
    cout << first;
    if constexpr (sizeof...(args) > 0) cout << ' ';
    println_impl(cout, args...);
}

inline void println() { cout << '\n'; }

// 输出 vector，支持指定 ostream
template <typename T>
void print_vec(ostream &o, const vector<T> &v, const string &sep = " ") {
    for (int i = 0; i < (int)v.size(); i++) {
        if (i) o << sep;
        o << v[i];
    }
    o << '\n';
}

template <typename T>
void print_vec(const vector<T> &v, const string &sep = " ") {
    print_vec(cout, v, sep);
}

// 输出边集
inline void print_edges(ostream &o, const vector<pair<int, int>> &edges) {
    for (auto [u, v] : edges) o << u << ' ' << v << '\n';
}

inline void print_edges(const vector<pair<int, int>> &edges) {
    print_edges(cout, edges);
}

// 输出浮点数 vector，指定精度
template <typename T>
void print_real_vec(ostream &o, const vector<T> &v, int precision = 6) {
    o << fixed << setprecision(precision);
    for (int i = 0; i < (int)v.size(); i++) {
        if (i) o << ' ';
        o << v[i];
    }
    o << '\n';
}

template <typename T>
void print_real_vec(const vector<T> &v, int precision = 6) {
    print_real_vec(cout, v, precision);
}

// 单文件多组数据：先写 T，再依次调 gen_one 写每组（每组自带换行）。
// 用法：write_multi_case(o, T, [&](ostream &g){ g << n << '\n'; print_vec(g, a); });
inline void write_multi_case(ostream &o, int T,
                             const function<void(ostream &)> &gen_one) {
    o << T << '\n';
    for (int i = 0; i < T; i++) gen_one(o);
}

// 创建目录（跨平台）；失败时抛异常（含路径和原因），
// 避免目录建不出来还继续"成功"地生成数据
inline void ensure_dir(const string &path) {
    error_code ec;
    fs::create_directories(path, ec);
    if (ec)
        throw runtime_error("ensure_dir: cannot create directory '" + path +
                            "': " + ec.message());
}

// ============================================================
// DataWriter — 自动写入 Data/{problem_id}_Data/ 目录
// ============================================================
struct DataWriter {
    string base_dir; // Data/{problem_id}_Data
    string prefix;
    string ext;
    int counter = 0;

    // 默认生成 001.in, 002.in, 003.in ...
    // 带前缀: DataWriter("in") -> in_001.in, in_002.in ...
    // 自定义后缀: DataWriter("in", "out") -> in_001.out, ...
    explicit DataWriter(const string &file_prefix = "", const string &file_ext = "in")
        : prefix(file_prefix), ext(file_ext) {
        if (!g_problem_id.empty()) {
            base_dir = "Data/" + g_problem_id + "_Data";
        } else {
            base_dir = "data";
        }
        ensure_dir(base_dir);
    }

    // 生成下一个文件路径
    string next_path() {
        counter++;
        ostringstream name;
        name << base_dir << "/";
        if (!prefix.empty()) name << prefix << "_";
        name << setw(3) << setfill('0') << counter << "." << ext;
        return name.str();
    }

    // 通过 lambda 写入（推荐）
    // 打开失败或写入失败会抛异常；确认写入成功才打印日志
    string next(const function<void(ostream &)> &writer) {
        string path = next_path();
        ofstream fout(path);
        if (!fout.is_open())
            throw runtime_error("DataWriter: cannot open file '" + path + "'");
        writer(fout);
        fout.flush();
        if (!fout)
            throw runtime_error("DataWriter: write failed: '" + path + "'");
        cerr << "  -> " << path << '\n';
        return path;
    }

    // 直接写入字符串内容
    string next_str(const string &content) {
        string path = next_path();
        ofstream fout(path);
        if (!fout.is_open())
            throw runtime_error("DataWriter: cannot open file '" + path + "'");
        fout << content;
        fout.flush();
        if (!fout)
            throw runtime_error("DataWriter: write failed: '" + path + "'");
        cerr << "  -> " << path << '\n';
        return path;
    }
};

// ============================================================
// gen_output — 读入 .in 文件，运行题解，生成 .out 文件
// ============================================================
// 用法：
//   gen_output([](istream &in, ostream &out) {
//       int n; in >> n;
//       // ... 你的题解逻辑 ...
//       out << ans << '\n';
//   });
//
// 会自动扫描 Data/{id}_Data/ 下的所有 .in 文件，
// 逐个读取、运行 solve、写出 .out 文件。
inline void gen_output(const function<void(istream &, ostream &)> &solve,
                       const string &dir_hint = "") {
    string dir = dir_hint;
    if (dir.empty()) {
        if (!g_problem_id.empty())
            dir = "Data/" + g_problem_id + "_Data";
        else
            dir = "data";
    }

    // 检查目录是否存在
    if (!fs::exists(dir)) {
        cerr << "Directory not found: " << dir << '\n';
        cerr << "Run without 'out' first to generate input files.\n";
        return;
    }

    // 收集所有 .in 文件，按文件名排序
    vector<fs::path> in_files;
    for (auto &entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".in") {
            in_files.push_back(entry.path());
        }
    }
    sort(in_files.begin(), in_files.end());

    if (in_files.empty()) {
        cerr << "No .in files found in " << dir << '\n';
        return;
    }

    for (auto &in_path : in_files) {
        // 读取输入
        ifstream fin(in_path);
        if (!fin) {
            cerr << "Failed to open: " << in_path << '\n';
            continue;
        }

        // 生成输出路径：xxx.in -> xxx.out
        fs::path out_path = in_path;
        out_path.replace_extension(".out");

        ofstream fout(out_path);
        if (!fout.is_open()) {
            cerr << "Failed to write: " << out_path << '\n';
            continue;
        }
        solve(fin, fout);

        cerr << "  [" << in_path.filename() << "] -> "
             << out_path.filename() << '\n';
    }

    cerr << "Done! " << in_files.size() << " output files generated.\n";
}

// ============================================================
// 测试用例组配置
// ============================================================
struct TestCase {
    string name;
    int n, m;
    long long max_val;
    map<string, long long> params;

    long long get(const string &key, long long default_val = 0) const {
        auto it = params.find(key);
        return it != params.end() ? it->second : default_val;
    }
};

// ============================================================
// 命令行参数解析
// ============================================================
struct Args {
    uint64_t seed = 0;
    bool seed_set = false;

    static Args parse(int argc, char *argv[]) {
        Args args;
        if (argc >= 2) {
            args.seed = stoull(argv[1]);
            args.seed_set = true;
        }
        return args;
    }
};
