/**
 * tests/test_gen_lib.cpp — gen_lib.h 生成器的属性测试
 *
 * 思路：对每个生成器反复调用（固定种子，可复现），断言所有"结构不变量"：
 *   随机数 → 边界/奇偶/倍数/不重复；数组/排列/字符串 → 合法性；
 *   树 → n-1 条边 + 连通 + 无自环 + 度数约束；图 → 边数 + 简单性 + 连通/无环/二分性。
 *
 * 只测"合法输入下输出合法"（非法输入走 assert，进程会中止，不适合在单测里测）。
 *
 * 编译运行（项目根目录）：cl /utf-8 /EHsc /std:c++17 /O2 /Fe:test_genlib.exe
 * tests/test_gen_lib.cpp && test_genlib，或 make test。退出码 0 = 全部通过。
 */

#include "../gen_lib.h"
using namespace std;

string g_problem_id = "genlib_selftest";

static int g_checks = 0, g_failed = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        g_checks++;                                                            \
        if (!(cond)) {                                                         \
            g_failed++;                                                        \
            cerr << "CHECK failed: " << #cond << "  (line " << __LINE__ << ")\n"; \
        }                                                                      \
    } while (0)

// ---------- 通用小工具 ----------

// 并查集：判连通
struct DSU {
    vector<int> p;
    DSU(int n) : p(n + 1) { iota(p.begin(), p.end(), 0); }
    int find(int x) { while (p[x] != x) x = p[x] = p[p[x]]; return x; }
    void uni(int a, int b) { p[find(a)] = find(b); }
};

// 树的通用检查：n-1 条边、无自环、u<v、连通；max_deg >= 0 时附带度数上界
static void check_tree(const vector<pair<int, int>> &edges, int n,
                       const string &name, int max_deg = -1) {
    CHECK((int)edges.size() == n - 1);
    DSU d(n);
    vector<int> deg(n + 1, 0);
    bool simple = true;
    for (auto [u, v] : edges) {
        if (u == v || u > v) simple = false;
        if (u < 1 || v > n) simple = false;
        deg[u]++, deg[v]++;
        d.uni(u, v);
    }
    CHECK(simple);
    int comp = 0;
    for (int i = 1; i <= n; i++)
        if (d.find(i) == i) comp++;
    CHECK(comp == 1);
    if (max_deg >= 0) {
        int mx = 0;
        for (int i = 1; i <= n; i++) mx = max(mx, deg[i]);
        CHECK(mx <= max_deg);
    }
    (void)name;
}

// 图的通用检查：恰好 m 条边、无自环、无重边
static void check_simple_graph(const vector<pair<int, int>> &edges, int m,
                               const string &name) {
    CHECK((int)edges.size() == m);
    set<pair<int, int>> seen;
    for (auto [u, v] : edges) {
        if (u == v) { CHECK(false); return; }
        auto key = minmax(u, v);
        if (!seen.insert({key.first, key.second}).second) { CHECK(false); return; }
    }
    (void)name;
}

// 有向图（允许 u>v）：恰好 m 条、无自环、无重边
static void check_directed(const vector<pair<int, int>> &edges, int m) {
    CHECK((int)edges.size() == m);
    set<pair<int, int>> seen;
    for (auto [u, v] : edges) {
        if (u == v) { CHECK(false); return; }
        if (!seen.insert({u, v}).second) { CHECK(false); return; }
    }
}

// ---------- 测试组 ----------

static void test_random_engine() {
    Random r(20260913); // 固定种子，可复现
    // 闭区间边界：各种正负范围
    vector<pair<long long, long long>> ranges = {
        {0, 0}, {7, 7}, {-5, 5}, {-100, -50}, {0, 1},
        {-1000000, 1000000}, {-(1LL << 62), (1LL << 62)},
    };
    for (auto [l, rr] : ranges) {
        for (int i = 0; i < 10000; i++) {
            long long x = r.next(l, rr);
            CHECK(l <= x && x <= rr);
        }
    }
    // 单参 / next_n
    for (int i = 0; i < 10000; i++) {
        long long x = r.next(100);
        CHECK(0 <= x && x <= 100);
        CHECK(r.next_n(1) == 0);
        CHECK(0 <= r.next_n(7) && r.next_n(7) < 7);
    }
    // 偶数 / 奇数 / 倍数（含负数区间）
    vector<tuple<long long, long long, long long>> mult_cases = {
        {-10, 10, 2}, {2, 2, 2}, {-4, -4, 2}, {0, 1000000, 2},
        {-99, 99, 2}, {1, 1000000, 7}, {-100, -1, 3}, {-7, 7, 10}, {5, 5, 5},
    };
    for (auto [l, rr, k] : mult_cases) {
        for (int i = 0; i < 5000; i++) {
            long long x = r.next_multiple(l, rr, k);
            CHECK(l <= x && x <= rr);
            CHECK(((x % k) + k) % k == 0);
        }
    }
    // 偶数/奇数：普通区间两个都测；单点区间只能测对应的那个奇偶性
    for (auto [l, rr] : vector<pair<long long, long long>>{
                             {-10, 10}, {0, 1000000}, {-99, 99}}) {
        for (int i = 0; i < 5000; i++) {
            long long e = r.next_even(l, rr), o = r.next_odd(l, rr);
            CHECK(l <= e && e <= rr && e % 2 == 0);
            CHECK(l <= o && o <= rr && o % 2 != 0);
        }
    }
    for (int i = 0; i < 100; i++) {
        CHECK(r.next_even(2, 2) == 2);
        CHECK(r.next_even(-4, -4) == -4);
        CHECK(r.next_odd(3, 3) == 3);
        CHECK(r.next_odd(-3, -3) == -3);
    }
    // 不重复集合
    for (auto [k, l, rr] : vector<tuple<int, long long, long long>>{
                               {5, 1, 10}, {10, 1, 10}, {0, 1, 5}, {1, 1, 1},
                               {50, 1, 100}, {1000, -500, 500}, {2000, -1000, 1000}}) {
        auto v = r.distinct(k, l, rr);
        CHECK((int)v.size() == k);
        set<long long> s(v.begin(), v.end());
        CHECK((int)s.size() == k);
        for (long long x : v) CHECK(l <= x && x <= rr);
    }
    // Floyd 采样：大范围小 k 也能 O(k) 完成（旧实现这里要物化 2^63 个数）
    {
        auto v = r.distinct(5, -(1LL << 62), (1LL << 62));
        CHECK((int)v.size() == 5);
        set<long long> s(v.begin(), v.end());
        CHECK((int)s.size() == 5);
        for (long long x : v) CHECK(-(1LL << 62) <= x && x <= (1LL << 62));
    }
    // k = 区间总量：恰好是整个区间的重排（不重不漏）
    {
        auto v = r.distinct(11, -5, 5);
        CHECK((int)v.size() == 11);
        set<long long> s(v.begin(), v.end());
        CHECK((int)s.size() == 11);
        for (long long x = -5; x <= 5; x++) CHECK(s.count(x) == 1);
    }
    // k = 0（含空区间 [1,0]：gen_partition(1,1) 会走到这里）
    CHECK(r.distinct(0, 1, 0).empty());
    CHECK(r.distinct(0, 1, 5).empty());
    // 均匀性粗检：单元素抽样各值出现频率接近期望
    {
        vector<long long> cnt(4, 0);
        for (int i = 0; i < 20000; i++) cnt[r.distinct(1, 10, 13)[0] - 10]++;
        for (long long c : cnt) CHECK(4000 <= c && c <= 6000);
    }
    // chance 的两个极端
    CHECK(r.chance(1.0));
    CHECK(!r.chance(0.0));
    // shuffle 保持多重集
    {
        vector<int> v(1000);
        iota(v.begin(), v.end(), 0);
        vector<int> sorted = v;
        r.shuffle(v);
        sort(v.begin(), v.end());
        CHECK(v == sorted);
    }
    // pick / sample（sample 按位置不放回 → 用值互异的样本，"不重复"才可断言）
    {
        vector<int> v = {3, 1, 4, 5, 9};
        bool picked = false;
        for (int i = 0; i < 100; i++) {
            int x = r.pick(v);
            picked = picked || (x == 9);
            CHECK(find(v.begin(), v.end(), x) != v.end());
        }
        CHECK(picked);
        auto s = r.sample(v, 3);
        CHECK((int)s.size() == 3);
        set<int> ss(s.begin(), s.end());
        CHECK((int)ss.size() == 3);
        for (int x : s) CHECK(find(v.begin(), v.end(), x) != v.end());
        // 边界：k = 0 / k = n
        CHECK(r.sample(v, 0).empty());
        vector<int> sorted_v = v;
        sort(sorted_v.begin(), sorted_v.end());
        auto all = r.sample(v, (int)v.size());
        sort(all.begin(), all.end());
        CHECK(all == sorted_v);
        vector<int> empty_v;
        CHECK(r.sample(empty_v, 0).empty());
    }
    // 同种子 → 同序列；不同种子 → 不同序列
    {
        Random a(42), b(42), c(43);
        bool same = true, diff = false;
        for (int i = 0; i < 1000; i++) {
            // 每个引擎每次迭代只抽一次——同表达式里重复抽会让消耗进度错位
            long long x = a.next(0, 1000000), y = b.next(0, 1000000),
                      z = c.next(0, 1000000);
            same = same && (x == y);
            diff = diff || (x != z);
        }
        CHECK(same);
        CHECK(diff);
    }
}

static void test_arrays_and_strings() {
    // 数组边界
    for (int t = 0; t < 200; t++) {
        auto v = gen_array(50, -7, 13);
        for (long long x : v) CHECK(-7 <= x && x <= 13);
        auto sv = gen_sorted_array(50, -7, 13);
        CHECK(is_sorted(sv.begin(), sv.end()));
        auto inc = gen_strictly_increasing(20, 1, 100);
        CHECK(is_sorted(inc.begin(), inc.end()));
        CHECK(adjacent_find(inc.begin(), inc.end()) == inc.end()); // 无重复
        auto uniq = gen_array_unique(20, 1, 100);
        set<long long> us(uniq.begin(), uniq.end());
        CHECK((int)us.size() == 20);
        auto reals = gen_real_array(10, -1.0, 1.0);
        for (double x : reals) CHECK(-1.0 <= x && x < 1.0);
        auto same = gen_array_same(10, 42);
        for (long long x : same) CHECK(x == 42);
    }
    // 分割：和为 n、k 份、每份 ≥ 1
    for (auto [n, k] : vector<pair<int, int>>{{1, 1}, {5, 5}, {100, 1}, {100, 50}, {100000, 777}}) {
        for (int t = 0; t < 200; t++) {
            auto p = gen_partition(n, k);
            CHECK((int)p.size() == k);
            long long sum = 0;
            for (int x : p) {
                CHECK(x >= 1);
                sum += x;
            }
            CHECK(sum == n);
        }
    }
    // 排列
    for (auto n : vector<int>{1, 2, 10, 1000}) {
        for (int t = 0; t < 100; t++) {
            auto p = gen_permutation(n);
            CHECK((int)p.size() == n);
            vector<int> sorted = p;
            sort(sorted.begin(), sorted.end());
            vector<int> expect(n);
            iota(expect.begin(), expect.end(), 1);
            CHECK(sorted == expect);
            auto rv = gen_permutation_reverse(n);
            for (int i = 0; i < n; i++) CHECK(rv[i] == n - i);
            auto almost = gen_permutation_almost_sorted(n, 3);
            CHECK((int)almost.size() == n);
            sort(almost.begin(), almost.end());
            CHECK(almost == expect);
        }
    }
    // 字符串
    for (int t = 0; t < 200; t++) {
        auto s = gen_string(37);
        CHECK((int)s.size() == 37);
        for (char c : s) CHECK('a' <= c && c <= 'z');
        auto g = gen_string(23, "ACGT");
        for (char c : g) CHECK(string("ACGT").find(c) != string::npos);
        for (auto n : vector<int>{1, 2, 3, 4, 5, 50}) {
            auto p = gen_palindrome(n);
            CHECK((int)p.size() == n);
            string rev(p.rbegin(), p.rend());
            CHECK(p == rev);
        }
        auto d = gen_string_distinct(26);
        set<char> ds(d.begin(), d.end());
        CHECK((int)ds.size() == 26);
    }
}

static void test_trees() {
    // prufer 随机树：覆盖小、中、大规模
    for (auto n : vector<int>{1, 2, 3, 4, 5, 17, 100, 1000, 100000})
        check_tree(gen_tree_prufer(n), n, "prufer");
    // 菊花 / 链的精确形状
    {
        auto star = gen_tree_star(5);
        CHECK((int)star.size() == 4);
        for (auto [u, v] : star) CHECK(u == 1);
        auto chain = gen_tree_chain(5);
        for (int i = 0; i < 4; i++) {
            CHECK(chain[i].first == i + 1);
            CHECK(chain[i].second == i + 2);
        }
    }
    // 度数限制树：d=2 必须恰好是链（两个度 1 端点，其余度 2）
    for (auto [n, d] : vector<pair<int, int>>{{1, 2}, {2, 2}, {5, 2}, {100, 2}, {1000, 3}, {300, 5}}) {
        auto e = gen_tree_deg_capped(n, d);
        check_tree(e, n, "deg_capped", d);
        if (d == 2 && n >= 2) {
            vector<int> deg(n + 1, 0);
            for (auto [u, v] : e) deg[u]++, deg[v]++;
            int ones = 0;
            for (int i = 1; i <= n; i++) ones += (deg[i] == 1);
            CHECK(ones == 2);
        }
    }
    // 随机二叉树：以 1 为根，每个节点至多 2 个孩子
    for (auto n : vector<int>{1, 2, 3, 7, 100, 1000}) {
        auto e = gen_tree_binary(n);
        check_tree(e, n, "binary");
        vector<vector<int>> adj(n + 1);
        for (auto [u, v] : e) adj[u].push_back(v), adj[v].push_back(u);
        vector<int> child(n + 1, 0), vis(n + 1, 0);
        queue<int> q;
        q.push(1), vis[1] = 1;
        while (!q.empty()) {
            int u = q.front();
            q.pop();
            for (int v : adj[u])
                if (!vis[v]) vis[v] = 1, child[u]++, q.push(v);
        }
        for (int i = 1; i <= n; i++) CHECK(child[i] <= 2);
    }
    // 父节点数组：能还原出一棵合法的树
    for (auto n : vector<int>{1, 2, 3, 100, 1000}) {
        auto p = gen_parent_array(n);
        CHECK((int)p.size() == (n >= 2 ? n - 1 : 0));
        if (n >= 2) {
            vector<pair<int, int>> e;
            for (int i = 2; i <= n; i++) {
                int parent = p[i - 2];
                CHECK(1 <= parent && parent <= n && parent != i);
                e.push_back({min(parent, i), max(parent, i)});
            }
            check_tree(e, n, "parent_array");
        }
    }
}

static void test_graphs() {
    // 连通无向图：边数精确 + 简单 + 连通（覆盖稀疏路径与稠密路径）
    for (auto [n, m] : vector<pair<int, int>>{
                             {1, 0}, {2, 1}, {3, 3}, {5, 4}, {5, 10}, {10, 45},
                             {50, 200}, {1000, 400000}, {1000, 499500},
                             {2000, 1500000}, {10000, 15000}}) {
        auto e = gen_graph_connected(n, m);
        check_simple_graph(e, m, "connected");
        DSU d(n);
        for (auto [u, v] : e) d.uni(u, v);
        int comp = 0;
        for (int i = 1; i <= n; i++)
            if (d.find(i) == i) comp++;
        CHECK(comp == 1);
    }
    // DAG：边数精确 + 简单 + 无环（Kahn 拓扑排序能排完）
    for (auto [n, m] : vector<pair<int, int>>{
                             {1, 0}, {2, 1}, {3, 2}, {3, 3}, {5, 10}, {60, 1770},
                             {100, 4950}, {1000, 3000}}) {
        auto e = gen_dag(n, m);
        check_directed(e, m);
        int N = n;
        vector<vector<int>> g(N + 1);
        vector<int> indeg(N + 1, 0);
        for (auto [u, v] : e) g[u].push_back(v), indeg[v]++;
        queue<int> q;
        for (int i = 1; i <= N; i++)
            if (indeg[i] == 0) q.push(i);
        int done = 0;
        while (!q.empty()) {
            int u = q.front();
            q.pop(), done++;
            for (int v : g[u])
                if (--indeg[v] == 0) q.push(v);
        }
        CHECK(done == N);
    }
    // 有向图
    for (auto [n, m] : vector<pair<int, int>>{{1, 0}, {2, 1}, {3, 6}, {100, 500}, {500, 2000}}) {
        check_directed(gen_graph_directed(n, m), m);
    }
    // 二分图：所有边都横跨两侧
    for (auto [n1, n2, m] : vector<tuple<int, int, int>>{
                                {1, 1, 1}, {2, 3, 6}, {5, 5, 7}, {10, 10, 100}, {7, 3, 9}}) {
        auto e = gen_graph_bipartite(n1, n2, m);
        CHECK((int)e.size() == m);
        set<pair<int, int>> seen;
        for (auto [u, v] : e) {
            CHECK(1 <= u && u <= n1);
            CHECK(n1 + 1 <= v && v <= n1 + n2);
            CHECK(seen.insert({u, v}).second);
        }
    }
    // 完全图：边数 = C(n,2)
    {
        auto e = gen_graph_complete(5);
        CHECK((int)e.size() == 10);
        check_simple_graph(e, 10, "complete");
    }
}

static void test_data_writer() {
    // 命名规则：[prefix_]NNN.ext
    fs::remove_all("Data/genlib_selftest_Data");
    {
        DataWriter dw("", "in");
        dw.next([](ostream &o) { o << "x\n"; });
        dw.next_str("y\n");
        CHECK(dw.counter == 2);
        CHECK(fs::exists("Data/genlib_selftest_Data/001.in"));
        CHECK(fs::exists("Data/genlib_selftest_Data/002.in"));
    }
    {
        DataWriter dw("pre", "out");
        dw.next([](ostream &o) { o << "z\n"; });
        CHECK(fs::exists("Data/genlib_selftest_Data/pre_001.out"));
    }
    // 内容一致性：next 写入的字节 = lambda 写出的内容
    {
        DataWriter dw("", "txt");
        dw.next([](ostream &o) { o << "1 2 3" << '\n'; });
        ifstream fin("Data/genlib_selftest_Data/001.txt");
        ostringstream ss;
        ss << fin.rdbuf();
        CHECK(ss.str() == "1 2 3\n");
    }
    fs::remove_all("Data/genlib_selftest_Data");
}

int main() {
    Random *saved = rnd;
    rnd = new Random(987654321); // 全局 rnd 也固定种子，测试可复现
    test_random_engine();
    test_arrays_and_strings();
    test_trees();
    test_graphs();
    test_data_writer();
    delete rnd;
    rnd = saved;

    cerr << "\n" << g_checks << " checks, " << g_failed << " failed.\n";
    if (g_failed) {
        cerr << "TEST FAILED\n";
        return 1;
    }
    cerr << "ALL TESTS PASSED\n";
    return 0;
}
