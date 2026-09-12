/**
 * tests/test_duipai.cpp — duipai.h 的自测
 *
 * 覆盖：输出比较的边界情况、核心流程（通过 / 不一致 / 异常 / 自定义比较器）、
 *       每轮种子独立性（可复现性）、stop_on_fail 行为、参数解析。
 *
 * 编译运行（项目根目录）：
 *   cl /utf-8 /EHsc /std:c++17 /O2 /Fe:test_duipai.exe tests/test_duipai.cpp && test_duipai
 *   或 make test
 * 退出码 0 = 全部通过。现场文件写在 Data/duipai_selftest_Duipai/（跑完自动删）。
 */

#include <cmath>

#include "../duipai.h"
using namespace std;

string g_problem_id = "duipai_selftest";

static int g_checks = 0, g_failed = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        g_checks++;                                                            \
        if (!(cond)) {                                                         \
            g_failed++;                                                        \
            cerr << "CHECK failed: " << #cond << "  (line " << __LINE__ << ")\n"; \
        }                                                                      \
    } while (0)

// 读整个文件（验证现场文件内容用）
static string read_file(const string &path) {
    ifstream fin(path, ios::binary);
    if (!fin) return "<file not found: " + path + ">";
    ostringstream ss;
    ss << fin.rdbuf();
    return ss.str();
}

int main() {
    const string dir = "Data/duipai_selftest_Duipai";
    error_code ec;
    fs::remove_all(dir, ec); // 清掉上次残留

    // ============ 一、输出比较 ============
    // 等价的情况
    CHECK(duipai_equal("", ""));
    CHECK(duipai_equal("\n", ""));
    CHECK(duipai_equal("1\n", "1"));            // 末尾换行可有可无
    CHECK(duipai_equal("1 2\n", "1 2 \n"));     // 行尾空格
    CHECK(duipai_equal("1\r\n2\r\n", "1\n2"));  // Windows 换行
    CHECK(duipai_equal("a\n\n\n", "a\n"));      // 文末空行
    CHECK(duipai_equal("a \n \n", "a\n"));      // 行尾空白 + 空白行
    CHECK(duipai_equal("1\t2\n", "1\t2"));      // 中间 Tab 保留
    // 不等价的情况
    CHECK(!duipai_equal("1 2", "1  2"));        // 行内空格数量
    CHECK(!duipai_equal("1\n2", "1\n3"));
    CHECK(!duipai_equal("1\n", "1\n\n2"));      // 中间空行
    CHECK(!duipai_equal("a b", "a b c"));
    CHECK(!duipai_equal("1\n2\n3", "1\n2\n3\n4"));
    CHECK(!duipai_equal(" 1", "1"));            // 行首空白参与比较
    CHECK(!duipai_equal("-0", "0"));
    // first diff 的行号和内容
    {
        string a, b;
        CHECK(duipai_first_diff("1\n2\n3", "1\n5\n3", a, b) == 2 && a == "2" && b == "5");
        CHECK(duipai_first_diff("1\n", "1\n2", a, b) == 2 && a == "(missing)" && b == "2");
        CHECK(duipai_first_diff("same", "same", a, b) == 0);
        CHECK(duipai_first_diff("", "", a, b) == 0);
    }

    // ============ 二、核心流程 ============
    DuipaiOptions opt;
    opt.save_dir = dir;

    // 1) 正确的 solve/brute → 全部通过
    {
        auto gen = [](ostream &o) { o << "2\n3\n"; };
        DuipaiSolver A{"solve", [](const string &in) {
            istringstream i(in);
            long long a, b;
            i >> a >> b;
            ostringstream o;
            o << a + b << '\n';
            return o.str();
        }};
        DuipaiSolver B{"brute", [](const string &in) {
            istringstream i(in);
            long long s = 0, x;
            while (i >> x) s += x;
            ostringstream o;
            o << s << '\n';
            return o.str();
        }};
        CHECK(run_duipai_core(50, 12345, gen, A, B, opt) == 0);
    }

    // 2) solve 有 bug → 第 1 轮就抓到，现场文件内容正确
    {
        auto gen = [](ostream &o) { o << "2\n3\n"; }; // 输入固定，失败必在第 1 轮
        DuipaiSolver A{"solve", [](const string &in) {
            istringstream i(in);
            long long a, b;
            i >> a >> b;
            ostringstream o;
            o << a * b << '\n'; // bug：写成乘法
            return o.str();
        }};
        DuipaiSolver B{"brute", [](const string &in) {
            istringstream i(in);
            long long s = 0, x;
            while (i >> x) s += x;
            ostringstream o;
            o << s << '\n';
            return o.str();
        }};
        CHECK(run_duipai_core(10, 999, gen, A, B, opt) == 1);
        CHECK(read_file(dir + "/001.in") == "2\n3\n");
        CHECK(read_file(dir + "/001.solve.out") == "6\n");
        CHECK(read_file(dir + "/001.brute.out") == "5\n");
    }

    // 3) solve 抛异常 → 按失败处理；正常的一边照常保存输出
    {
        auto gen = [](ostream &o) { o << "1\n"; };
        DuipaiSolver A{"solve", [](const string &) -> string {
            throw runtime_error("boom");
        }};
        DuipaiSolver B{"brute", [](const string &) { return string("0\n"); }};
        CHECK(run_duipai_core(3, 7, gen, A, B, opt) == 1);
        CHECK(read_file(dir + "/001.in") == "1\n");
        CHECK(read_file(dir + "/001.brute.out") == "0\n");
        CHECK(!fs::exists(dir + "/001.solve.out")); // 抛异常的一边没有输出文件
    }

    // 4) 每轮种子独立：同一种子下，前 k 轮的数据与总轮数无关
    {
        vector<string> run5, run10;
        auto gen5 = [&](ostream &o) {
            long long v = rnd->next(1, 1000000);
            run5.push_back(to_string(v));
            o << v << '\n';
        };
        auto gen10 = [&](ostream &o) {
            long long v = rnd->next(1, 1000000);
            run10.push_back(to_string(v));
            o << v << '\n';
        };
        // 输出 = 输入原样返回 → 必然一致，纯测 gen 的可复现性
        DuipaiSolver A{"solve", [](const string &in) { return in; }};
        DuipaiSolver B{"brute", [](const string &in) { return in; }};
        CHECK(run_duipai_core(5, 777, gen5, A, B, opt) == 0);
        CHECK(run_duipai_core(10, 777, gen10, A, B, opt) == 0);
        CHECK(run10.size() == 10);
        for (int i = 0; i < 5; i++) CHECK(run10[i] == run5[i]);
        // 换种子 → 数据应当不同
        vector<string> run5b;
        auto gen5b = [&](ostream &o) {
            long long v = rnd->next(1, 1000000);
            run5b.push_back(to_string(v));
            o << v << '\n';
        };
        CHECK(run_duipai_core(5, 778, gen5b, A, B, opt) == 0);
        bool any_diff = false;
        for (int i = 0; i < 5; i++)
            if (run5b[i] != run5[i]) any_diff = true;
        CHECK(any_diff);
    }

    // 5) stop_on_fail = false：跑完所有轮，每轮失败各留一份现场
    {
        auto gen = [](ostream &o) { o << "1\n"; };
        DuipaiSolver A{"solve", [](const string &) { return string("wrong\n"); }};
        DuipaiSolver B{"brute", [](const string &) { return string("right\n"); }};
        DuipaiOptions opt2 = opt;
        opt2.stop_on_fail = false;
        CHECK(run_duipai_core(7, 42, gen, A, B, opt2) == 1);
        for (int r = 1; r <= 7; r++)
            CHECK(fs::exists(dir + "/" + duipai_tag(r) + ".in"));
    }

    // 6) 自定义比较器（浮点误差比较）
    {
        auto gen = [](ostream &o) { o << "0.5\n"; };
        DuipaiSolver A{"solve", [](const string &) { return string("0.500001\n"); }};
        DuipaiSolver B{"brute", [](const string &) { return string("0.5\n"); }};
        DuipaiOptions opt3 = opt;
        CHECK(run_duipai_core(1, 1, gen, A, B, opt3) == 1); // 内置规则：不等
        opt3.compare = [](const string &x, const string &y) {
            istringstream a(x), b(y);
            double u = 0, v = 0;
            a >> u;
            b >> v;
            return fabs(u - v) < 1e-3;
        };
        CHECK(run_duipai_core(1, 1, gen, A, B, opt3) == 0); // 自定义：等价
    }

    // 7) gen_case 抛异常 → 失败，且半成品输入已落盘
    {
        auto gen = [](ostream &o) {
            o << "partial";
            throw runtime_error("gen broke");
        };
        DuipaiSolver A{"solve", [](const string &in) { return in; }};
        DuipaiSolver B{"brute", [](const string &in) { return in; }};
        CHECK(run_duipai_core(2, 5, gen, A, B, opt) == 1);
        CHECK(read_file(dir + "/001.in") == "partial");
    }

    // 8) 空输入 + 空输出 → 通过（引擎只打 warning）
    {
        auto gen = [](ostream &) {};
        DuipaiSolver A{"solve", [](const string &) { return string(); }};
        DuipaiSolver B{"brute", [](const string &) { return string(); }};
        CHECK(run_duipai_core(1, 3, gen, A, B, opt) == 0);
    }

    // 9) rounds = 0：什么都不跑，退出码 0
    {
        auto gen = [](ostream &o) { o << "1\n"; };
        DuipaiSolver A{"solve", [](const string &) { return string("x\n"); }};
        DuipaiSolver B{"brute", [](const string &) { return string("y\n"); }};
        CHECK(run_duipai_core(0, 9, gen, A, B, opt) == 0);
    }

    // ============ 三、参数解析 ============
    {
        uint64_t v = 0;
        CHECK(duipai_parse_u64("0", v) && v == 0);
        CHECK(duipai_parse_u64("12345", v) && v == 12345);
        CHECK(duipai_parse_u64("18446744073709551615", v) && v == 18446744073709551615ULL);
        CHECK(!duipai_parse_u64("-1", v));               // 负号拒绝（stoull 会回绕！）
        CHECK(!duipai_parse_u64("12ab", v));             // 尾部垃圾拒绝（stoull 会截断！）
        CHECK(!duipai_parse_u64(" 12", v));
        CHECK(!duipai_parse_u64("", v));
        CHECK(!duipai_parse_u64("99999999999999999999999", v)); // 溢出
        CHECK(!duipai_parse_u64("18446744073709551616", v));    // 刚好超 1 位
    }

    // 清理现场
    fs::remove_all(dir, ec);

    cerr << "\n" << g_checks << " checks, " << g_failed << " failed.\n";
    if (g_failed) {
        cerr << "TEST FAILED\n";
        return 1;
    }
    cerr << "ALL TESTS PASSED\n";
    return 0;
}
