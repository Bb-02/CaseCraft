/**
 * newcase.cpp — 一键生成题目目录脚手架
 *
 * 用法：
 *   newcase                  交互式：依次询问题目英文名、中文标题
 *   newcase Sum              题目英文名 = Sum
 *   newcase Sum 求和         英文名 + 中文标题一步到位
 *
 * 生成：
 *   Generators/Sum/Sum.cpp   复制 template.cpp，g_problem_id 已填好
 *   Generators/Sum/题面.md   题面骨架
 *
 * 编译：
 *   cl /utf-8 /EHsc /std:c++17 /O2 /Fe:newcase.exe newcase.cpp
 *   g++ -std=c++17 -O2 newcase.cpp -o newcase
 *
 * 注意：需在项目根目录运行（要读取 template.cpp）。
 */

#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32) && defined(_MSC_VER)
#include <windows.h>
#endif

namespace fs = std::filesystem;
using namespace std;

// 题目名只允许字母、数字、下划线、连字符（要做目录名和 g_problem_id）
bool valid_id(const string &s) {
    if (s.empty()) return false;
    for (char c : s)
        if (!isalnum((unsigned char)c) && c != '_' && c != '-') return false;
    return true;
}

string read_file(const fs::path &p) {
    ifstream fin(p, ios::binary);
    ostringstream ss;
    ss << fin.rdbuf();
    return ss.str();
}

// 题面骨架
string make_statement_md(const string &id, const string &title) {
    ostringstream oss;
    oss << "# " << (title.empty() ? id : title) << "\n\n";
    oss << "## 题目描述\n\n（在这里写题面）\n\n";
    oss << "## 输入格式\n\n一行一个整数 n（1 ≤ n ≤ 100000）。\n\n";
    oss << "## 输出格式\n\n（描述输出；无解输出 -1 之类）\n\n";
    oss << "## 样例\n\n";
    oss << "### 输入样例 1\n\n```\n\n```\n\n";
    oss << "### 输出样例 1\n\n```\n\n```\n\n";
    oss << "## 数据范围\n\n";
    oss << "对于 100% 的数据，1 ≤ n ≤ 100000。\n";
    return oss.str();
}

// 从 template.cpp 生成题目源码：替换 include 路径和题目 ID
string make_cpp_source(const string &tmpl, const string &id, bool &ok) {
    ok = true;
    string src = tmpl;

    // 生成器在子目录里，include 路径要改成 ../../
    // gen_lib.h 是必须存在的；duipai.h / bench.h 是新模板才有的，没有就跳过
    const pair<string, string> rewrites[] = {
        {"#include \"gen_lib.h\"", "#include \"../../gen_lib.h\""},
        {"#include \"duipai.h\"", "#include \"../../duipai.h\""},
        {"#include \"bench.h\"", "#include \"../../bench.h\""},
    };
    for (auto [from, to] : rewrites) {
        size_t pos = src.find(from);
        if (pos == string::npos) {
            if (from.find("gen_lib") != string::npos) {
                cerr << "错误：template.cpp 中找不到 " << from << '\n';
                ok = false;
                return src;
            }
            continue;
        }
        src.replace(pos, from.size(), to);
    }

    const string id_from = "\"my_problem\"";
    const string id_to = "\"" + id + "\"";
    size_t pos = src.find(id_from);
    if (pos == string::npos) {
        cerr << "错误：template.cpp 中找不到 g_problem_id 的默认值 my_problem\n";
        ok = false;
        return src;
    }
    src.replace(pos, id_from.size(), id_to);
    return src;
}

#if defined(_WIN32) && defined(_MSC_VER)
// MSVC 下 Windows 的 main argv 按系统 ANSI 编码（GBK）解释，中文参数会乱码；
// 用 wmain 拿 UTF-16 参数再手动转 UTF-8。
// MinGW 的 g++ 默认链接 ANSI 入口（wmain 需要 -municode），走下面的普通 main。
string wide_to_utf8(const wchar_t *ws) {
    if (!ws) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, ws, -1, nullptr, 0, nullptr, nullptr);
    string s(n > 1 ? n - 1 : 0, '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, ws, -1, s.data(), n, nullptr, nullptr);
    return s;
}
#endif

int run(const vector<string> &args) {
    int argc = (int)args.size();

    string id, title;
    if (argc >= 2) id = args[1];
    if (argc >= 3) title = args[2];

    if (id.empty()) {
        cout << "题目英文名（目录名 + 数据 ID，如 Sum）: ";
        getline(cin, id);
    }
    if (argc < 3) {
        cout << "中文标题（直接回车跳过）: ";
        getline(cin, title);
    }

    if (!valid_id(id)) {
        cerr << "错误：题目名只能包含字母、数字、下划线、连字符: " << id << '\n';
        return 1;
    }

    fs::path tmpl_path = "template.cpp";
    if (!fs::exists(tmpl_path)) {
        cerr << "错误：找不到 template.cpp，请在项目根目录运行\n";
        return 1;
    }
    string tmpl = read_file(tmpl_path);

    bool ok = false;
    string src = make_cpp_source(tmpl, id, ok);
    if (!ok) return 1;

    fs::path dir = fs::path("Generators") / id;
    if (fs::exists(dir)) {
        cerr << "错误：目录已存在，不覆盖: Generators/" << id << '\n';
        return 1;
    }
    error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
        cerr << "错误：无法创建目录 Generators/" << id << ": " << ec.message() << '\n';
        return 1;
    }

    // 文件名含中文时必须用宽字符构造 path，
    // 否则窄字符串会按系统 ANSI 编码（GBK）解释导致乱码文件名
    {
        ofstream fout(dir / (id + ".cpp"), ios::binary);
        fout << src;
    }
    {
        ofstream fout(dir / L"题面.md", ios::binary);
        fout << make_statement_md(id, title);
    }

    cout << "\n已创建 Generators/" << id << "/\n";
    cout << "  -> " << id << ".cpp   （g_problem_id = \"" << id << "\"）\n";
    cout << "  -> 题面.md\n\n";
    cout << "下一步：\n";
    cout << "  1. 编辑 Generators/" << id << "/题面.md 和 " << id << ".cpp\n";
    cout << "  2. 在根目录编译：cl /utf-8 /EHsc /std:c++17 /O2 /Fe:gen.exe Generators/" << id << '/' << id << ".cpp\n";
    cout << "  3. 运行 ./gen 生成输入，./gen out 生成输出（数据进 Data/" << id << "_Data/）\n";
    return 0;
}

#if defined(_WIN32) && defined(_MSC_VER)
int wmain(int argc, wchar_t *argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    vector<string> args;
    for (int i = 0; i < argc; i++) args.push_back(wide_to_utf8(argv[i]));
    return run(args);
}
#else
int main(int argc, char *argv[]) {
    vector<string> args;
    for (int i = 0; i < argc; i++) args.push_back(argv[i]);
    return run(args);
}
#endif
