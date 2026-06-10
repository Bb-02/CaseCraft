/**
 * template.cpp — 数据生成模板
 *
 * 用法：
 *   生成输入数据：./gen [seed]
 *   生成输出数据：./gen out [seed]
 *   检查外部程序：./gen check ./my_program
 *   一键对拍：./gen duipai ./my_program [seed]
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
void print_usage(const char *prog) {
    cerr << "Usage:\n";
    cerr << "  " << prog << " [seed]                         Generate .in files\n";
    cerr << "  " << prog << " out [seed]                     Generate .out files\n";
    cerr << "  " << prog << " check <program> [--strict]     Run program and compare with .out\n";
    cerr << "  " << prog << " duipai <program> [seed] [--strict]\n";
}

int main(int argc, char *argv[]) {
    string mode = "in";
    string program_path;
    uint64_t seed = 0;
    bool seed_set = false;
    bool strict = false;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];

        if (arg == "out") {
            mode = "out";
        } else if (arg == "check") {
            mode = "check";
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            program_path = argv[++i];
        } else if (arg == "duipai" || arg == "test") {
            mode = "duipai";
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            program_path = argv[++i];
        } else if (arg == "--strict") {
            strict = true;
        } else {
            try {
                seed = stoull(arg);
                seed_set = true;
            } catch (...) {
                cerr << "Unknown argument: " << arg << '\n';
                print_usage(argv[0]);
                return 1;
            }
        }
    }

    rnd = seed_set ? new Random(seed) : new Random();

    cerr << "Problem: " << g_problem_id << '\n';
    if (seed_set) cerr << "Seed: " << seed << '\n';

    int failed = 0;

    if (mode == "out") {
        gen_output(solve);
    } else if (mode == "check") {
        failed = check_outputs(program_path, "", strict);
    } else if (mode == "duipai") {
        generate_input();
        gen_output(solve);
        failed = check_outputs(program_path, "", strict);
    } else {
        generate_input();
    }

    delete rnd;
    return failed == 0 ? 0 : 1;
}
