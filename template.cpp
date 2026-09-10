/**
 * template.cpp — 数据生成模板
 *
 * 用法：
 *   生成输入数据：./gen [seed]
 *   生成输出数据：./gen out
 *
 * 数据输出到：Data/{g_problem_id}_Data/
 */

#include "gen_lib.h"
using namespace std;

// ============================================================
// && 改这里 — 题目 ID
// ============================================================
string g_problem_id = "my_problem";

// ============================================================
// && 步骤 1：造输入数据 (.in)
// ============================================================
void generate_input() {
    DataWriter dw; // 不加参数 → 文件名为 001.in, 002.in ...

    // ------ 示例 1：简单的一行数据 ------
    dw.next([](ostream &o) {
        o << "1 2\n";           // 就写一行
    });

    // ------ 示例 2：n + n个数 ------
    dw.next([](ostream &o) {
        int n = rnd->next(1, 10);    // n 是 1~10 的随机数
        o << n << '\n';               // 第一行：n
        auto a = gen_array(n, 1, 100);// 生成 n 个 [1,100] 的数
        print_vec(o, a);              // 第二行：a1 a2 ... an
    });

    // ------ 示例 3：n + n个数，带负数 ------
    dw.next([](ostream &o) {
        int n = rnd->next(5, 20);
        o << n << '\n';
        auto a = gen_array(n, -1000, 1000); // [-1000, 1000]，有正有负
        print_vec(o, a);
    });

    // ------ 示例 4：浮点数 ------
    dw.next([](ostream &o) {
        int n = 5;
        o << n << '\n';
        auto f = gen_real_array(n, -1.0, 1.0); // 5 个 [-1.0, 1.0) 的浮点数
        print_real_vec(o, f, 6);               // 保留 6 位小数输出
    });

    // ------ 示例 5：排列（1~n 随机打乱） ------
    dw.next([](ostream &o) {
        int n = 8;
        o << n << '\n';
        auto p = gen_permutation(n); // 1..n 的随机排列
        print_vec(o, p);
    });

    // ------ 示例 6：一个文件里多组询问 ------
    dw.next([](ostream &o) {
        int t = rnd->next(3, 8); // 询问组数
        o << t << '\n';
        while (t--) {
            int l = rnd->next(1, 50);
            int r = rnd->next(l, 100);
            o << l << ' ' << r << '\n';
        }
    });

    // ------ 示例 7：树 ------
    dw.next([](ostream &o) {
        int n = 10;
        o << n << '\n';
        auto edges = gen_tree_prufer(n); // 随机树
        print_edges(o, edges);           // 每行 "u v"
    });

    // ------ 示例 8：循环批量生成 ------
    for (int i = 0; i < 5; i++) {
        dw.next([&](ostream &o) { // [&] 捕获外层的 i
            int n = rnd->next(100, 500);
            o << n << '\n';
            print_vec(o, gen_array(n, 1, 1'000'000));
        });
    }

    cerr << "Input done! " << dw.counter << " files -> Data/"
         << g_problem_id << "_Data/\n";
}

// ============================================================
// && 步骤 2：题解代码（in = 输入流，out = 输出流）
// ============================================================
void solve(istream &in, ostream &out) {
    // 示例：读两个数，输出和（换成你真正的题解）
    int a, b;
    in >> a >> b;
    out << a + b << '\n';
}

// ============================================================
int main(int argc, char *argv[]) {
    bool out_mode = false;
    uint64_t seed = 0;
    bool seed_set = false;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "out") out_mode = true;
        else seed = stoull(arg), seed_set = true;
    }

    // ---- 种子初始化说明 ----
    // Random 底层是 mt19937_64 伪随机引擎：
    //   - 同一种子 → 完全相同的随机序列（可复现数据，方便出题调参）
    //   - 不同种子 → 完全不同的随机序列
    //
    // 种子生成策略：
    //   1. 用户指定种子（如 ./gen 12345）→ 混入题目 ID 后作为种子
    //      hash("题目ID") ^ 用户种子 → 保证不同题目即使同种子，序列也不同
    //   2. 未指定种子 → 用系统时钟 + 题目 ID 自动生成（每次运行都不同）
    //
    // 为什么要混入 g_problem_id：
    //   如果两个题目都写 rnd->next(1, 100) 作为第一个随机调用，且用同一种子，
    //   它们会得到相同的"随机"值。混入题目 ID 后彻底隔离，互不干扰。
    if (seed_set) {
        // 用户种子 XOR 题目 ID 哈希 → 不同题目的序列彻底隔离
        uint64_t id_hash = hash<string>{}(g_problem_id);
        rnd = new Random(seed ^ id_hash);
        cerr << "Problem: " << g_problem_id << '\n';
        cerr << "Seed: " << seed << " (mixed with id hash: " << id_hash << ")\n";
    } else {
        // 无种子 → 默认构造函数用 steady_clock 时间戳做种子
        rnd = new Random();
        cerr << "Problem: " << g_problem_id << '\n';
        cerr << "Seed: <random from system clock>\n";
    }

    try {
        if (out_mode) gen_output(solve);
        else generate_input();
    } catch (const exception &e) {
        // 目录建不出来/文件写不进去等 I/O 错误：打印原因并以非零码退出，
        // 方便脚本及时终止，避免误以为数据已生成
        cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    delete rnd;
    return 0;
}
