#pragma once
/**
 * duipai.h — 对拍器（配合 gen_lib.h 使用）
 *
 * 在题目 .cpp 里实现三个函数，就可以用暴力解自动验证高效解：
 *   void gen_case(ostream &o);              // 造一个【小】随机数据（brute 要跑得动）
 *   void solve(istream &in, ostream &out);  // 被测的高效解（模板里已有）
 *   void brute(istream &in, ostream &out);  // 正确性显然的暴力解，输出格式与 solve 完全一致
 *
 * 命令行（在项目根目录编译运行）：
 *   ./gen duipai               默认跑 100 轮（随机种子）
 *   ./gen duipai 1000          跑 1000 轮
 *   ./gen duipai 1000 2024     跑 1000 轮，固定种子 2024（可复现）
 *
 * 每一轮：
 *   1. 用独立种子造一个小数据，先写入 Data/{id}_Duipai/cur.in（防死循环丢现场）
 *   2. 同一份输入分别喂给 solve 和 brute
 *   3. 输出不一致 → 保存 005.in / 005.solve.out / 005.brute.out 并停止（退出码 1）
 *
 * 比较规则：按行比较；忽略每行行尾空白和文末空行；行首空白、中间空行、
 *           行内空白数量都参与比较（偏严格，图形输出安全）。
 *
 * 说明：
 *   - 每轮种子只由 (总种子, 轮数) 决定 → 加大轮数重跑，前面轮次的数据一字不差
 *   - 对拍在同一进程内跑，没有超时机制；真死循环就 Ctrl+C，cur.in 就是肇事数据
 *   - solve/brute/gen_case 抛异常也按"发现失败"处理，现场照存
 *   - 每次运行会先清空 Data/{id}_Duipai/ 再开始，现场永远是本次运行的
 *   - 想自定义比较（比如浮点误差）→ 调 run_duipai_core 并设置
 *     DuipaiOptions::compare
 *   - 想接入外部 exe → 再写一个 DuipaiSolver 适配器即可，核心不用改
 */

#include "gen_lib.h"

// ============================================================
// 用户接口约定 —— 在题目 .cpp 中实现这三个函数
// ============================================================
// 这里只做声明。注意：新模板的 main 里引用了 run_duipai_cli（它又引用这三个函数），
// 所以编译新模板的题目时必须给出 gen_case 和 brute 的定义，
// 否则报链接错误 undefined reference —— 那是在提醒你"对拍三件套"还没写全。
extern void gen_case(ostream &o);
extern void solve(istream &in, ostream &out);
extern void brute(istream &in, ostream &out);

// ============================================================
// 输出比较
// ============================================================

// 按行切分 + 规范化：去掉每行行尾空白（空格/Tab/\r），去掉文末空行。
// 行首空白、中间空行、行内空白都原样保留参与比较。
inline vector<string> duipai_lines(const string &s) {
    vector<string> lines;
    string cur;
    for (char c : s) {
        if (c == '\n') {
            lines.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) lines.push_back(cur); // 末尾没有换行也算一行
    for (string &l : lines)
        while (!l.empty() && (l.back() == ' ' || l.back() == '\t' || l.back() == '\r'))
            l.pop_back();
    while (!lines.empty() && lines.back().empty())
        lines.pop_back();
    return lines;
}

// 两份输出是否等价（规则见文件头注释）
inline bool duipai_equal(const string &a, const string &b) {
    vector<string> la = duipai_lines(a), lb = duipai_lines(b);
    if (la.size() != lb.size()) return false;
    for (size_t i = 0; i < la.size(); i++)
        if (la[i] != lb[i]) return false;
    return true;
}

// 找第一处不同的行（1-based；完全等价返回 0）。
// diff_a / diff_b 写入该行内容（超长截断到 60 字符），某一方没有该行时为 "(missing)"
inline long long duipai_first_diff(const string &a, const string &b,
                                   string &diff_a, string &diff_b) {
    auto trunc60 = [](const string &s) -> string {
        if (s.size() <= 60) return s;
        return s.substr(0, 57) + "...";
    };
    vector<string> la = duipai_lines(a), lb = duipai_lines(b);
    size_t n = max(la.size(), lb.size());
    for (size_t i = 0; i < n; i++) {
        string x = i < la.size() ? la[i] : "(missing)";
        string y = i < lb.size() ? lb[i] : "(missing)";
        if (x != y) {
            diff_a = trunc60(x);
            diff_b = trunc60(y);
            return (long long)(i + 1);
        }
    }
    return 0;
}

// ============================================================
// 现场保存
// ============================================================

// 二进制模式写文件：内存里是什么字节，文件里就是什么字节（不被 Windows 换行转换干扰）
inline void duipai_write_file(const string &path, const string &content) {
    ofstream fout(path, ios::binary);
    if (!fout.is_open())
        throw runtime_error("duipai: cannot open file '" + path + "'");
    fout << content;
    fout.flush();
    if (!fout)
        throw runtime_error("duipai: write failed: '" + path + "'");
}

// 3 位编号：1 -> "001"，1234 -> "1234"（与 DataWriter 的编号规则一致）
inline string duipai_tag(int round) {
    ostringstream os;
    os << setw(3) << setfill('0') << round;
    return os.str();
}

// 把输入的前几行打到 stderr，不切目录也能大概看出事数据长什么样
inline void duipai_preview_input(const string &input, int max_lines = 5) {
    vector<string> lines = duipai_lines(input);
    if (lines.empty()) return;
    cerr << "  input preview (" << min<int>((int)lines.size(), max_lines)
         << "/" << lines.size() << " lines):\n";
    for (int i = 0; i < (int)lines.size() && i < max_lines; i++) {
        string l = lines[i].size() > 60 ? lines[i].substr(0, 57) + "..." : lines[i];
        cerr << "    | " << l << '\n';
    }
}

// ============================================================
// 对拍核心 —— 由参数驱动，方便单元测试和扩展
// （以后想加"调用外部 exe"的适配器，只要再写一个 DuipaiSolver，核心不用动）
// ============================================================

// 一个"解题器"：输入文本 -> 输出文本；抛异常 = 运行出错
struct DuipaiSolver {
    string name; // 用在失败现场的文件名里（如 005.solve.out），别带路径分隔符
    function<string(const string &)> run;
};

struct DuipaiOptions {
    string save_dir;           // 现场目录；留空 = Data/{g_problem_id}_Duipai
    bool stop_on_fail = true;  // true: 第一处失败立刻停；false: 跑完所有轮并统计
    // 自定义比较器（返回 true = 等价）；留空 = 内置规则（duipai_equal）
    function<bool(const string &, const string &)> compare;
};

// splitmix64：把 (总种子, 轮数) 充分混匀，保证每轮种子相互独立、无规律。
// 不能直接用 base+round 之类的线性组合——那样相邻轮的随机序列会相关。
inline uint64_t duipai_splitmix64(uint64_t x) {
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

inline uint64_t duipai_round_seed(uint64_t base_seed, int round) {
    return duipai_splitmix64(base_seed ^
                             duipai_splitmix64(0x9E3779B97F4A7C15ULL * (uint64_t)round));
}

// 离开作用域时把全局 rnd 删干净并置空——所有 return / 异常路径都覆盖，
// 不会泄漏，也不会让外面的 main 二次 delete
struct DuipaiRndGuard {
    ~DuipaiRndGuard() { delete rnd; rnd = nullptr; }
};

// 把 solve/brute 这类 (istream, ostream) 函数包成 DuipaiSolver
inline DuipaiSolver duipai_fn_solver(const string &name,
                                     function<void(istream &, ostream &)> fn) {
    DuipaiSolver s;
    s.name = name;
    s.run = [fn](const string &input) -> string {
        istringstream in(input);
        ostringstream out;
        fn(in, out);
        return out.str();
    };
    return s;
}

// 对拍主循环。
//   rounds / base_seed : 轮数与总种子（每轮种子 = duipai_round_seed(base_seed, round)）
//   a / b              : 两个解题器，习惯上 a = solve（被测），b = brute（参照）
// 返回值：0 = 全部通过；1 = 有失败
// 注意：会接管全局 rnd —— 每轮换成独立种子的新引擎，结束时置空。
inline int run_duipai_core(long long rounds, uint64_t base_seed,
                           function<void(ostream &)> gen_case_fn,
                           const DuipaiSolver &a, const DuipaiSolver &b,
                           const DuipaiOptions &opt = {}) {
    string dir = opt.save_dir.empty() ? "Data/" + g_problem_id + "_Duipai"
                                      : opt.save_dir;
    // 每次运行清空现场目录再重建：绝对不留上一次运行的过期文件
    // （否则上次是输出不一致、这次是抛异常，过期的 .solve.out 会误导排查）
    error_code ec;
    fs::remove_all(dir, ec);
    ensure_dir(dir);
    cerr << "Duipai dir: " << dir << "/\n";

    delete rnd; // 接管全局引擎（若之前建过，先释放）
    rnd = nullptr;
    DuipaiRndGuard guard;

    long long fail_count = 0;
    for (int round = 1; round <= rounds; round++) {
        delete rnd;
        rnd = new Random(duipai_round_seed(base_seed, round));

        // ---- 1. 造数据（gen_case 抛异常也算失败，异常前写出的内容照样落盘）----
        string input, gen_err;
        {
            ostringstream oss;
            try {
                gen_case_fn(oss);
            } catch (const exception &e) {
                gen_err = string("gen_case threw exception: ") + e.what();
            } catch (...) {
                gen_err = "gen_case threw unknown exception";
            }
            // str() 必须放在 catch 之后：半成品输入是排查 gen_case 的重要线索
            input = oss.str();
        }

        string tag = duipai_tag(round);
        // 先落盘再跑程序：万一 solve/brute 死循环，Ctrl+C 之后 cur.in 就是肇事数据
        duipai_write_file(dir + "/cur.in", input);

        auto report_fail = [&](const string &reason) {
            fail_count++;
            cerr << "FAILED at round " << round << "/" << rounds << ": " << reason << '\n';
            duipai_preview_input(input);
        };

        // 造数据失败：不跑解题器（畸形输入上的输出没有意义），只存输入现场
        if (!gen_err.empty()) {
            report_fail(gen_err);
            duipai_write_file(dir + "/" + tag + ".in", input);
            cerr << "  saved -> " << dir << "/" << tag << ".in\n";
            if (opt.stop_on_fail) return 1;
            continue;
        }

        if (input.empty())
            cerr << "warning: round " << round << " generated an empty input\n";

        // ---- 2. 同一份输入分别跑两个解（异常按失败处理）----
        string out_a, out_b, err_a, err_b;
        bool ok_a = true, ok_b = true;
        try {
            out_a = a.run(input);
        } catch (const exception &e) {
            ok_a = false;
            err_a = a.name + " threw exception: " + e.what();
        } catch (...) {
            ok_a = false;
            err_a = a.name + " threw unknown exception";
        }
        try {
            out_b = b.run(input);
        } catch (const exception &e) {
            ok_b = false;
            err_b = b.name + " threw exception: " + e.what();
        } catch (...) {
            ok_b = false;
            err_b = b.name + " threw unknown exception";
        }

        // ---- 3. 判定 & 留现场 ----
        if (!ok_a || !ok_b) {
            string reason;
            if (!err_a.empty()) reason += err_a;
            if (!err_b.empty()) {
                if (!reason.empty()) reason += "; ";
                reason += err_b;
            }
            report_fail(reason);
            duipai_write_file(dir + "/" + tag + ".in", input);
            cerr << "  saved -> " << dir << "/" << tag << ".in";
            // 正常跑完的那一边，输出照常保存；抛异常的一边没有可靠输出，不保存
            if (ok_a) {
                duipai_write_file(dir + "/" + tag + "." + a.name + ".out", out_a);
                cerr << ", " << tag << "." << a.name << ".out";
            }
            if (ok_b) {
                duipai_write_file(dir + "/" + tag + "." + b.name + ".out", out_b);
                cerr << ", " << tag << "." << b.name << ".out";
            }
            cerr << '\n';
            if (opt.stop_on_fail) return 1;
            continue;
        }

        // 完全相同的字节直接通过，省一次规范化
        bool same = out_a == out_b ||
                    (opt.compare ? opt.compare(out_a, out_b) : duipai_equal(out_a, out_b));
        if (!same) {
            report_fail("outputs differ");
            duipai_write_file(dir + "/" + tag + ".in", input);
            duipai_write_file(dir + "/" + tag + "." + a.name + ".out", out_a);
            duipai_write_file(dir + "/" + tag + "." + b.name + ".out", out_b);
            cerr << "  saved -> " << dir << "/" << tag << ".in, " << tag << "."
                 << a.name << ".out, " << tag << "." << b.name << ".out\n";
            string da, db;
            long long line_no = duipai_first_diff(out_a, out_b, da, db);
            if (line_no > 0) {
                cerr << "  first diff at line " << line_no << ":\n";
                cerr << "    " << a.name << " | " << da << '\n';
                cerr << "    " << b.name << " | " << db << '\n';
            } else {
                // 内置规则看不出差别 → 一定是自定义比较器判的
                cerr << "  (custom comparator says different; built-in rules see no diff)\n";
            }
            if (opt.stop_on_fail) return 1;
            continue;
        }

        if (round % 100 == 0)
            cerr << "  round " << round << "/" << rounds << " ok\n";
    }

    if (fail_count == 0)
        cerr << "Duipai passed! " << rounds << "/" << rounds << " rounds.\n";
    else
        cerr << "Duipai finished: " << (rounds - fail_count) << "/" << rounds
             << " passed, " << fail_count << " failed. Artifacts in '" << dir << "/'\n";
    return fail_count == 0 ? 0 : 1;
}

// ============================================================
// 命令行入口：./gen duipai [rounds] [seed]
// ============================================================

// 解析无符号十进制整数，只接受纯数字。
// 不直接用 stoull：它会把 "-1" 静默回绕成大正数、把 "12ab" 解析成 12，
// 这些都会变成"轮数=天文数字"之类的离奇行为；溢出也要拒绝。
inline bool duipai_parse_u64(const string &s, uint64_t &out) {
    if (s.empty() || s.size() > 20) return false;
    for (char c : s)
        if (c < '0' || c > '9') return false;
    try {
        size_t pos = 0;
        unsigned long long v = stoull(s, &pos);
        if (pos != s.size()) return false;
        out = (uint64_t)v;
        return true;
    } catch (...) {
        return false;
    }
}

// main 里对拍分支调用：nums 是去掉模式词后剩下的参数
inline int run_duipai_cli(const vector<string> &nums) {
    long long rounds = 100;
    uint64_t seed = 0;
    bool seed_set = false;

    if (nums.size() > 2) {
        cerr << "用法: ./gen duipai [rounds] [seed]\n";
        return 1;
    }
    if (nums.size() >= 1) {
        uint64_t r;
        if (!duipai_parse_u64(nums[0], r) || r > 1000000000ULL) {
            cerr << "错误: 无效的轮数 '" << nums[0] << "'（应为 0 ~ 10^9 的整数）\n";
            return 1;
        }
        rounds = (long long)r;
    }
    if (nums.size() >= 2) {
        if (!duipai_parse_u64(nums[1], seed)) {
            cerr << "错误: 无效的种子 '" << nums[1] << "'\n";
            return 1;
        }
        seed_set = true;
    }

    uint64_t base_seed;
    cerr << "Problem: " << g_problem_id << '\n';
    if (seed_set) {
        // 与造数据模式同一套约定：混入题目 ID 哈希，不同题目同种子也互不干扰
        uint64_t id_hash = hash<string>{}(g_problem_id);
        base_seed = seed ^ id_hash;
        cerr << "Seed: " << seed << " (mixed with id hash: " << id_hash << ")\n";
    } else {
        base_seed = (uint64_t)chrono::steady_clock::now().time_since_epoch().count();
        cerr << "Seed: <random from system clock>\n";
    }
    cerr << "Rounds: " << rounds << '\n';

    return run_duipai_core(rounds, base_seed, gen_case,
                           duipai_fn_solver("solve", solve),
                           duipai_fn_solver("brute", brute));
}
