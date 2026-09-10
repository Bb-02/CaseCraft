CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

# 编译模板
template: template.cpp gen_lib.h
	$(CXX) $(CXXFLAGS) template.cpp -o gen_template

# 编译示例
example: example_graph.cpp gen_lib.h
	$(CXX) $(CXXFLAGS) example_graph.cpp -o gen_graph

# 通用编译：make gen SRC=Generators/Sum/Sum.cpp
gen:
	$(CXX) $(CXXFLAGS) $(SRC) -o gen

# 运行
run: gen
	./gen $(SEED)

# 一键建题工具：make newcase 后运行 ./newcase Sum
newcase: newcase.cpp
	$(CXX) $(CXXFLAGS) newcase.cpp -o newcase

# 清理所有生成数据
clean:
	rm -f gen gen_template gen_graph newcase
	rm -rf Data/*_Data

.PHONY: template example gen run newcase clean
