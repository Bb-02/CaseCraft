/**
 * tests/test_bench.cpp — bench.h 的自测
 *
 * 覆盖：共享输入公平性、种子确定性、输出交叉校验、异常处理、
 *       统计量与表格结构、CLI 参数解析。
 *
 * 编译运行（项目根目录）：
 *   cl /utf-8 /EHsc /std:c++17 /O2 /Fe:test_bench.exe tests/test_bench.cpp && test_bench
 *   或 make test
 * 退出码 0 = 全部通过。
 */

#include "../bench.h"
using namespace std;

string g_problem_id = "bench_selftest";

// 提供给 run_bench_cli 的最小实现（走合法参数路径时会用到）
void gen_bench(ostream &o) { o << "1\n"; }
static void dummy_alg(istream &in, ostream &out) {
    int x;
    in >> x;
    out << x << '\n';
}
void bench_register(vector<BenchEntry> &v) { v.push_back({"solve", dummy_alg}); }

static int g_checks = 0, g_failed = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        g_checks++;                                                            \
        if (!(cond)) {                                                         \
            g_failed++;                                                        \
            cerr << "CHECK failed: " << #cond << "  (line " << __LINE__ << ")\n"; \
        }                                                                      \
    } while (0)

// 造一个可感知工作量的实现：做 w 轮 1e6 次整数运算，耗时大致与 w 成正比。
// 输出只取决于输入（不含 s）——模拟"多个实现算同一答案、只是快慢不同"。
static function<void(istream &, ostream &)> burn(int w) {
    return [w](istream &in, ostream &out) {
        int x;
        in >> x;
        volatile long long s = 0; // volatile：防止整段循环被编译器优化掉
        for (int i = 0; i < w; i++)
            for (int j = 0; j < 1000000; j++) s += j;
        out << x << '\n';
    };
}

int main() {
    ostringstream log;

    // ============ 一、核心流程 ============
    DuipaiRndGuard rnd_guard; // 测试里 core 会接管 rnd，退出前清干净

    // 1) 两个实现、输出一致 → 退出码 0，表格含两个名字和 1.00x
    {
        auto gen = [](ostream &o) { o << "7\n"; };
        vector<BenchEntry> entries = {{"fast", burn(1)}, {"slow", burn(3)}};
        log.str("");
        log.clear();
        int rc = run_bench_core(2, 12345, gen, entries, log);
        CHECK(rc == 0);
        CHECK(log.str().find("fast") != string::npos);
        CHECK(log.str().find("slow") != string::npos);
        CHECK(log.str().find("1.00x") != string::npos); // 第一个实现是基准
        CHECK(log.str().find("ERROR") == string::npos);
        CHECK(log.str().find("警告") == string::npos); // 输出一致，没有警告
    }

    // 2) 公平性：所有实现拿到的输入逐字节相同
    {
        vector<string> seen_a, seen_b;
        auto gen = [](ostream &o) { o << rnd->next(1, 1000000) << '\n'; };
        BenchEntry a{"a", [&](istream &in, ostream &) {
                         string l;
                         getline(in, l);
                         seen_a.push_back(l);
                     }};
        BenchEntry b{"b", [&](istream &in, ostream &) {
                         string l;
                         getline(in, l);
                         seen_b.push_back(l);
                     }};
        vector<BenchEntry> entries = {a, b};
        run_bench_core(3, 777, gen, entries, log);
        CHECK(seen_a.size() == 4);  // 预热 1 + 计时 3
        CHECK(seen_b.size() == 4);
        for (int i = 0; i < 4; i++) CHECK(seen_a[i] == seen_b[i]);
        // 且输入与种子绑定（确定性）
        CHECK(seen_a[0] == seen_a[3]);
    }

    // 3) 种子确定性：同种子 → 同输入；换种子 → 不同输入
    {
        vector<string> run1, run2, run3;
        auto gen1 = [&](ostream &o) {
            long long v = rnd->next(1, 1000000);
            run1.push_back(to_string(v));
            o << v << '\n';
        };
        auto gen2 = [&](ostream &o) {
            long long v = rnd->next(1, 1000000);
            run2.push_back(to_string(v));
            o << v << '\n';
        };
        auto gen3 = [&](ostream &o) {
            long long v = rnd->next(1, 1000000);
            run3.push_back(to_string(v));
            o << v << '\n';
        };
        BenchEntry rec{"rec", [](istream &in, ostream &) {
                           string l;
                           getline(in, l);
                       }};
        vector<BenchEntry> entries = {rec};
        CHECK(run_bench_core(1, 42, gen1, entries, log) == 0);
        CHECK(run_bench_core(1, 42, gen2, entries, log) == 0);
        CHECK(run_bench_core(1, 43, gen3, entries, log) == 0);
        CHECK(run1.size() == 1 && run2.size() == 1 && run3.size() == 1);
        CHECK(run1[0] == run2[0]);  // 同种子 → 同输入
        CHECK(run1[0] != run3[0]);  // 换种子 → 不同输入
    }

    // 4) 输出不一致 → 退出码 1 + 警告
    {
        auto gen = [](ostream &o) { o << "1\n"; };
        BenchEntry a{"a", [](istream &in, ostream &out) {
                         int x;
                         in >> x;
                         out << x << '\n';
                     }};
        BenchEntry b{"b", [](istream &in, ostream &out) {
                         int x;
                         in >> x;
                         out << x + 1 << '\n'; // 错的
                     }};
        vector<BenchEntry> entries = {a, b};
        log.str("");
        log.clear();
        CHECK(run_bench_core(1, 1, gen, entries, log) == 1);
        CHECK(log.str().find("输出不一致") != string::npos);
        CHECK(log.str().find("b vs a") != string::npos);
    }

    // 5) 实现抛异常 → ERROR 行 + 退出码 1，另一个实现照常计时
    {
        auto gen = [](ostream &o) { o << "1\n"; };
        BenchEntry a{"a", [](istream &in, ostream &out) {
                         int x;
                         in >> x;
                         out << x << '\n';
                     }};
        BenchEntry bad{"bad", [](istream &, ostream &) -> void {
                           throw runtime_error("boom");
                       }};
        vector<BenchEntry> entries = {a, bad};
        log.str("");
        log.clear();
        CHECK(run_bench_core(2, 9, gen, entries, log) == 1);
        CHECK(log.str().find("ERROR (exception: boom)") != string::npos);
        CHECK(log.str().find("1.00x") != string::npos); // a 照常出表
    }

    // 6) 基准抛异常时，基准顺延到下一个可用实现
    {
        auto gen = [](ostream &o) { o << "1\n"; };
        BenchEntry bad{"bad", [](istream &, ostream &) -> void {
                           throw runtime_error("dead");
                       }};
        BenchEntry a{"a", [](istream &in, ostream &out) {
                         int x;
                         in >> x;
                         out << x << '\n';
                     }};
        vector<BenchEntry> entries = {bad, a};
        log.str("");
        log.clear();
        CHECK(run_bench_core(1, 5, gen, entries, log) == 1);
        CHECK(log.str().find("ERROR") != string::npos);
        CHECK(log.str().find("1.00x") != string::npos); // a 顶上做基准
    }

    // 7) 空注册列表 → 退出码 1
    {
        auto gen = [](ostream &o) { o << "1\n"; };
        vector<BenchEntry> empty_entries;
        CHECK(run_bench_core(1, 1, gen, empty_entries, log) == 1);
    }

    // 8) 单实现也能测（绝对耗时场景）：正常出表
    {
        auto gen = [](ostream &o) { o << "1\n"; };
        vector<BenchEntry> entries = {{"only", burn(1)}};
        log.str("");
        log.clear();
        CHECK(run_bench_core(1, 8, gen, entries, log) == 0);
        CHECK(log.str().find("only") != string::npos);
        CHECK(log.str().find("1.00x") != string::npos);
    }

    // ============ 二、CLI 参数解析 ============
    {
        CHECK(run_bench_cli({"abc"}) == 1);          // 非数字
        CHECK(run_bench_cli({"0"}) == 1);            // repeats 必须 >= 1
        CHECK(run_bench_cli({"-5"}) == 1);           // 负号拒绝
        CHECK(run_bench_cli({"1", "2", "3"}) == 1);  // 参数过多
        CHECK(run_bench_cli({"1", "42"}) == 0);      // 合法：1 遍 + 种子
        CHECK(run_bench_cli({}) == 0);               // 合法：全默认
    }

    cerr << "\n" << g_checks << " checks, " << g_failed << " failed.\n";
    if (g_failed) {
        cerr << "TEST FAILED\n";
        return 1;
    }
    cerr << "ALL TESTS PASSED\n";
    return 0;
}
