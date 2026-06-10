CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

# 编译模板
template: template.cpp gen_lib.h
	$(CXX) $(CXXFLAGS) template.cpp -o gen_template

# 编译示例
example: example_graph.cpp gen_lib.h
	$(CXX) $(CXXFLAGS) example_graph.cpp -o gen_graph

# 通用编译：make gen SRC=your_file.cpp
gen:
	$(CXX) $(CXXFLAGS) $(SRC) -o gen

# 运行
run: gen
	./gen $(SEED)

# 生成 .out
out: gen
	./gen out $(SEED)

# 对拍：make check SRC=sum.cpp SOL=main.cpp SEED=12345
check:
	$(CXX) $(CXXFLAGS) $(SRC) -o gen
	$(CXX) $(CXXFLAGS) $(SOL) -o user_prog
	./gen duipai ./user_prog $(SEED)

# 清理所有生成数据
clean:
	rm -f gen gen_template gen_graph user_prog
	rm -rf Data/*_Data

.PHONY: template example gen run out check clean
