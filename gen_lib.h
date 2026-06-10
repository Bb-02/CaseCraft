#pragma once
/**
 * gen_lib.h — 算法题数据生成核心库
 *
 * 用法：
 *   1. 复制 template.cpp 到新文件
 *   2. 修改 g_problem_id = "你的题目名"
 *   3. 编写 generate() 函数
 *   4. 编译运行：g++ -std=c++17 -O2 your_file.cpp -o gen && ./gen [seed]
 *
 * 生成的数据自动放入 Data/{problem_id}_Data/ 目录。
 */

#include <algorithm>
#include <cassert>
#include <chrono>
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

    // 随机挑选一个元素
    template <typename T> const T &pick(const vector<T> &v) {
        assert(!v.empty());
        return v[next_n(v.size())];
    }

    // 不放回地挑选 k 个元素
    template <typename T> vector<T> sample(const vector<T> &v, int k) {
        assert(0 <= k && k <= (int)v.size());
        vector<int> idx(v.size());
        iota(idx.begin(), idx.end(), 0);
        std::shuffle(idx.begin(), idx.end(), engine);
        vector<T> res(k);
        for (int i = 0; i < k; i++) res[i] = v[idx[i]];
        return res;
    }

    // 生成唯一的随机整数集合（[l, r] 中取 k 个不重复的）
    vector<long long> distinct(int k, long long l, long long r) {
        assert(k <= r - l + 1);
        if (k < (r - l + 1) / 3) {
            unordered_set<long long> used;
            vector<long long> res;
            while ((int)res.size() < k) {
                long long x = next(l, r);
                if (used.insert(x).second) res.push_back(x);
            }
            return res;
        }
        vector<long long> all(r - l + 1);
        iota(all.begin(), all.end(), l);
        shuffle(all);
        return vector<long long>(all.begin(), all.begin() + k);
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
    rnd->shuffle(vector<int>(p.begin(), p.begin() + mid));
    rnd->shuffle(vector<int>(p.begin() + mid, p.end()));
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
    set<int> cand;
    cand.insert(1);
    for (int v = 2; v <= n; v++) {
        int u = rnd->pick(vector<int>(cand.begin(), cand.end()));
        edges.push_back({min(u, v), max(u, v)});
        deg[u]++, deg[v]++;
        if (deg[u] >= max_deg) cand.erase(u);
        cand.insert(v);
    }
    return edges;
}

// n 个节点的随机二叉树（每个节点最多 2 个子节点，1 为根）
vector<pair<int, int>> gen_tree_binary(int n) {
    return gen_tree_deg_capped(n, 3); // 根最多 2 子 + 可能无父，deg <= 3
}

// ============================================================
// 图生成器
// ============================================================

// n 个节点 m 条边的简单无向连通图（n-1 <= m <= n*(n-1)/2）
vector<pair<int, int>> gen_graph_connected(int n, int m) {
    assert(m >= n - 1 && m <= (long long)n * (n - 1) / 2);
    auto edges = gen_tree_prufer(n);
    set<pair<int, int>> edge_set(edges.begin(), edges.end());
    while ((int)edges.size() < m) {
        int u = rnd->next(1, n), v = rnd->next(1, n);
        if (u == v) continue;
        auto e = make_pair(min(u, v), max(u, v));
        if (edge_set.count(e)) continue;
        edge_set.insert(e);
        edges.push_back(e);
    }
    rnd->shuffle(edges);
    return edges;
}

// n 个节点 m 条边的有向无环图
vector<pair<int, int>> gen_dag(int n, int m) {
    long long max_m = (long long)n * (n - 1) / 2;
    assert(m <= max_m);
    auto perm = gen_permutation(n);
    set<pair<int, int>> edge_set;
    while ((int)edge_set.size() < m) {
        int i = rnd->next_n(n), j = rnd->next_n(n);
        if (i >= j) continue;
        edge_set.insert({perm[i], perm[j]});
    }
    return vector<pair<int, int>>(edge_set.begin(), edge_set.end());
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
    set<pair<int, int>> edge_set;
    while ((int)edge_set.size() < m) {
        int u = rnd->next(1, n1);
        int v = rnd->next(n1 + 1, n1 + n2);
        edge_set.insert({u, v});
    }
    return vector<pair<int, int>>(edge_set.begin(), edge_set.end());
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

// 创建目录（跨平台）
inline void ensure_dir(const string &path) {
    error_code ec;
    fs::create_directories(path, ec);
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
    string next(const function<void(ostream &)> &writer) {
        string path = next_path();
        ofstream fout(path);
        writer(fout);
        cerr << "  -> " << path << '\n';
        return path;
    }

    // 直接写入字符串内容
    string next_str(const string &content) {
        string path = next_path();
        ofstream fout(path);
        fout << content;
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
        dir = "Data/" + g_problem_id + "_Data";
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
