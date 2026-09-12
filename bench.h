#pragma once
/**
 * bench.h — 计时横向测评（配合 gen_lib.h / duipai.h 使用）
 *
 * 对拍管"对不对"，bench 管"快不快"。用同一份输入横向对比多个实现的耗时，
 * 典型用途：测剪枝/换数据结构/算法改动前后的效率提升。
 *
 * 在题目 .cpp 里实现两个函数（newcase 生成的骨架自带空壳）：
 *   void gen_bench(ostream &o);                  // 造一份基准数据，所有实现共用
 *   void bench_register(vector<BenchEntry> &v);  // 注册待测实现，第一个是基准（1.00x）
 *
 * 命令行（在项目根目录）：
 *   ./gen bench              每个实现跑 5 遍（随机种子）
 *   ./gen bench 10           每个实现跑 10 遍
 *   ./gen bench 10 42        10 遍 + 固定种子 42（输入可复现，跨构建对比用它）
 *
 * 输出一张表：min / median / 相对基准的加速比（rel）。
 *
 * 内置正确性交叉校验：所有实现的输出按 duipai 的比较规则互比，
 * 不一致会警告并使退出码为 1 —— 输出都不一样，比时间没有意义。
 *
 * 公平性由工具保证的部分：
 *   - 所有实现共用同一份输入；每遍运行前全局 rnd 重置为同一初态（随机化算法也公平）
 *   - 预热 1 遍不计入统计；计时采用交错轮次（每轮所有实现各跑一遍），
 *     降低 CPU 降频/缓存状态漂移带来的先后偏差
 *   - 报 min 和 median，不报平均值（离群值会带偏平均）
 *   - 计时后把输出长度汇入 volatile 变量，防止编译器把无副作用的计算优化掉
 *
 * 工具保证不了的，靠使用纪律（同机对比、同样编译优化等级、看量级不看零头）。
 *
 * 说明：
 *   - bench.h 复用 duipai.h 的比较器和参数解析；依赖链 bench.h -> duipai.h -> gen_lib.h
 *   - 新模板的 main 引用了 run_bench_cli（它又引用 gen_bench/bench_register），
 *     所以编译时必须给出它们的定义，否则报链接错误 —— 提醒你还没写 bench 两件套
 *   - 只测耗时，不测内存；换数据规模请改 gen_bench 后重跑
 */

#include <cstdint>

#include "duipai.h"

// 一个待测实现：从输入流读到输出流（和 solve 同一形状）
struct BenchEntry {
    string name; // 显示在表格第一列
    function<void(istream &, ostream &)> run;
};

// ============================================================
// 用户接口约定 —— 在题目 .cpp 中实现这两个函数
// ============================================================
extern void gen_bench(ostream &o);
extern void bench_register(vector<BenchEntry> &v);

// ============================================================
// 计时核心
// ============================================================

// 防止死代码消除的"汇"：计时的计算结果在这里留下痕迹，
// 编译器就不敢把"没有副作用"的整段计算优化掉
inline volatile long long bench_sink = 0;

// 交错轮次的单次计时。运行前把全局 rnd 重置为固定初态，
// 保证随机化算法在所有实现、所有轮次下看到完全相同的随机序列。
// out_text 非空时把输出文本写进去（正确性交叉校验用），为空则丢弃——
// 纯计时路径不拷贝输出，大输出也不会拖慢总时长。返回耗时（毫秒）。
inline double bench_run_once(const BenchEntry &e, const string &input,
                             uint64_t run_seed, string *out_text = nullptr) {
    delete rnd;
    rnd = new Random(run_seed);
    auto t0 = chrono::steady_clock::now();
    istringstream in(input);
    ostringstream out;
    e.run(in, out);
    auto t1 = chrono::steady_clock::now();
    bench_sink += (long long)out.tellp(); // tellp O(1)，不复制输出内容
    double ms = chrono::duration<double, milli>(t1 - t0).count();
    if (out_text) *out_text = out.str();
    return ms;
}

// 偶数个样本取中间两个的平均
inline double bench_median(vector<double> v) {
    sort(v.begin(), v.end());
    size_t n = v.size();
    return n % 2 ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2.0;
}

// 计时主流程。
//   repeats   : 每个实现的计时次数（另有一次预热不计入）
//   base_seed : 总种子（输入与运行初态都由它派生）
//   gen_fn    : 造基准数据的函数（所有实现共用其输出）
//   entries   : 待测实现列表；第一个有有效计时的作为 1.00x 基准
//   log       : 报告输出流（默认 cerr，测试时可换成别的流）
// 返回值：0 = 全部正常且输出一致；1 = 有实现抛异常或输出不一致
inline int run_bench_core(int repeats, uint64_t base_seed,
                          function<void(ostream &)> gen_fn,
                          const vector<BenchEntry> &entries, ostream &log = cerr) {
    if (entries.empty()) {
        log << "错误: bench_register 没有注册任何实现\n";
        return 1;
    }

    // ---- 造一份共享输入 ----
    delete rnd;
    rnd = nullptr;
    DuipaiRndGuard guard;
    rnd = new Random(duipai_round_seed(base_seed, 0));
    string input;
    {
        ostringstream oss;
        try {
            gen_fn(oss);
        } catch (const exception &e) {
            log << "Error: gen_bench threw exception: " << e.what() << '\n';
            return 1;
        }
        input = oss.str();
    }
    if (input.empty()) log << "warning: gen_bench 造出的基准数据是空的\n";

    const uint64_t run_seed = duipai_round_seed(base_seed, 1);

    // ---- 预热一遍（不计入统计），输出留作正确性校验样本 ----
    struct Result {
        string name;
        bool threw = false;
        string err;
        vector<double> ms;
        string output;
    };
    vector<Result> res(entries.size());
    for (size_t k = 0; k < entries.size(); k++) {
        res[k].name = entries[k].name;
        try {
            bench_run_once(entries[k], input, run_seed, &res[k].output);
        } catch (const exception &e) {
            res[k].threw = true;
            res[k].err = e.what();
        } catch (...) {
            res[k].threw = true;
            res[k].err = "unknown exception";
        }
    }

    // ---- 交错计时：每轮把所有实现各跑一遍（输出丢弃，不拷贝）----
    for (int i = 0; i < repeats; i++) {
        for (size_t k = 0; k < entries.size(); k++) {
            if (res[k].threw) continue; // 预热就挂了的不再计时
            try {
                double ms = bench_run_once(entries[k], input, run_seed);
                res[k].ms.push_back(ms);
            } catch (const exception &e) {
                res[k].threw = true;
                res[k].err = e.what();
            } catch (...) {
                res[k].threw = true;
                res[k].err = "unknown exception";
            }
        }
    }

    // ---- 正确性交叉校验 ----
    bool mismatch = false;
    size_t ref_idx = SIZE_MAX;
    for (size_t k = 0; k < res.size(); k++)
        if (!res[k].threw) { ref_idx = k; break; }
    if (ref_idx != SIZE_MAX) {
        for (size_t k = ref_idx + 1; k < res.size(); k++) {
            if (res[k].threw) continue;
            if (!duipai_equal(res[k].output, res[ref_idx].output)) {
                mismatch = true;
                log << "警告: 输出不一致 —— " << res[k].name << " vs "
                    << res[ref_idx].name << "（输出不同的实现之间比时间没有意义，请先 ./gen duipai）\n";
            }
        }
    }

    // ---- 统计 ----
    vector<double> mins(res.size(), 0.0), meds(res.size(), 0.0);
    size_t base_idx = SIZE_MAX;
    for (size_t k = 0; k < res.size(); k++) {
        if (res[k].threw || res[k].ms.empty()) continue;
        mins[k] = *min_element(res[k].ms.begin(), res[k].ms.end());
        meds[k] = bench_median(res[k].ms);
        if (base_idx == SIZE_MAX) base_idx = k; // 第一个有有效计时的作为基准
    }

    // ---- 表格 ----
    auto fmt_ms = [](double ms) {
        ostringstream o;
        o << fixed << setprecision(2) << ms << " ms";
        return o.str();
    };
    size_t name_w = 4; // "name" 的宽度
    for (auto &r : res) name_w = max(name_w, r.name.size());

    log << "\n  " << left << setw(name_w) << "name" << "   "
        << right << setw(12) << "min" << setw(14) << "median" << setw(10) << "rel"
        << '\n';
    for (size_t k = 0; k < res.size(); k++) {
        log << "  " << left << setw(name_w) << res[k].name;
        if (res[k].threw) {
            log << "   ERROR (exception: " << res[k].err << ")\n";
            continue;
        }
        string rel;
        if (k == base_idx || base_idx == SIZE_MAX) {
            rel = "1.00x";
        } else if (mins[k] <= 0.0 || mins[base_idx] <= 0.0) {
            rel = "n/a";
        } else {
            ostringstream o;
            o << fixed << setprecision(2) << mins[base_idx] / mins[k] << "x";
            rel = o.str();
        }
        log << "   " << right << setw(12) << fmt_ms(mins[k])
            << setw(14) << fmt_ms(meds[k]) << setw(10) << rel << '\n';
    }
    log << "  (" << input.size() << " bytes input, " << repeats
        << " repeats + 1 warmup each, min/median per entry)\n";

    bool any_threw = false;
    for (auto &r : res)
        if (r.threw) any_threw = true;
    return (mismatch || any_threw) ? 1 : 0;
}

// ============================================================
// 命令行入口：./gen bench [repeats] [seed]
// ============================================================

inline int run_bench_cli(const vector<string> &nums) {
    long long repeats = 5;
    uint64_t seed = 0;
    bool seed_set = false;

    if (nums.size() > 2) {
        cerr << "用法: ./gen bench [repeats] [seed]\n";
        return 1;
    }
    if (nums.size() >= 1) {
        uint64_t r;
        if (!duipai_parse_u64(nums[0], r) || r < 1 || r > 100000) {
            cerr << "错误: 无效的重复次数 '" << nums[0] << "'（应为 1 ~ 100000 的整数）\n";
            return 1;
        }
        repeats = (long long)r;
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
        uint64_t id_hash = hash<string>{}(g_problem_id);
        base_seed = seed ^ id_hash;
        cerr << "Seed: " << seed << " (mixed with id hash: " << id_hash << ")\n";
    } else {
        base_seed = (uint64_t)chrono::steady_clock::now().time_since_epoch().count();
        cerr << "Seed: <random from system clock>\n";
    }
    cerr << "Repeats: " << repeats << '\n';

    vector<BenchEntry> entries;
    bench_register(entries);
    return run_bench_core((int)repeats, base_seed, gen_bench, entries);
}
