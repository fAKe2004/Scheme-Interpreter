# Extension 1 多线程优化设计说明

Yuxuan Wang
---

### 可优化部分：
I. Parse Phase:

  1. let/letrec 的 [var expr]* 解析
  2. begin 的 expr* 解析
  3. if 的 cond conseq alter 解析
  4. lambda 的 var* 解析，但是考虑到 var 解析复杂度很低，故不做多线程优化
  5. binary 的 rand1, rand2 解析。

II. Evaluation Phase:
  1. let/letrec 的 [var expr]* eval 部分
  2. begin 的 expr* 求值
  3. apply 的 parameters expr* 求值
  4. binary 的 rand1, rand2 求值。

---

### Concurrent Control & Other Implementation Issues


1. try/catch 只会在同一 thread 下生效，如果启动并发，需要一个 exception_flag 来确定是否有某个 thread 抛出了异常

这里应该没有内存时序控制问题，因为是 try catch 处理的，没有重排的可能性。

2. 对于变量域 env 的 extend 操作，需要保证多线程一致性，mutex 保障

update : 全部求值完再 sequentially update env 就好了。 不需要这个 env。如果有锁的话，并发效率不会比 sequential 高太多。

3. var 的求值部分？应该不存在并发控制问题，都是在 let/letrec/apply 过程求的，多个之间不会互相影响。

4. 坑：SharePtr is not thread-safe. 把 ref_cnt 的 int 改成 atomic\<int\>。

---

### Design

```cpp
// parallel.cpp
#include "parallel.hpp"

#ifdef PARALLEL_OPTIMIZE

void parse_mt(Syntax& stx, Assoc env /* copy */, Expr& result, std::atomic<bool>& exception_flag) {
  try {
    result = stx->parse(env);
  } catch(...) {
    exception_flag.store(true);
  }
}

void eval_mt(Expr& expr, Assoc env /* copy */, Value& result, std::atomic<bool>& exception_flag) {
  try {
    result = expr->eval(env);
  } catch(...) {
    exception_flag.store(true);
  }
}

// Parse: launch parallel parse processes for stxs
std::vector<Expr> parse_mt_launch(std::vector<Syntax>& stxs, const Assoc& env) {
  int n = stxs.size();
  std::atomic<bool> exception_flag(false);
  std::vector<std::thread> ths(n);
  std::vector<Expr> exprs(n, Expr(nullptr));
  for (int i = 0; i < n; i++) {
    ths[i] = std::thread(parse_mt, std::ref(stxs[i]), env, std::ref(exprs[i]), std::ref(exception_flag));
  }
  for (auto& th : ths) 
    if (th.joinable())
      th.join();
  if (exception_flag.load())
    throw RuntimeError("Parse Error");
  return exprs;
}

// Eval: launch parallel evaluation processes for exprs
std::vector<Value> eval_mt_launch(std::vector<Expr>& exprs, const Assoc& env) {
  int n = exprs.size();
  std::atomic<bool> exception_flag(false);
  std::vector<std::thread> ths(n);
  std::vector<Value> values(n, Value(nullptr));
  for (int i = 0; i < n; i++) {
    ths[i] = std::thread(eval_mt, std::ref(exprs[i]), env, std::ref(values[i]), std::ref(exception_flag));
  }
  for (auto& th : ths) 
    if (th.joinable())
      th.join();
  if (exception_flag.load())
    throw RuntimeError("Evaluation Error");
  return values;
}

#endif

```

其它对于 PARSE 和 EVAL 的修改见 `parser.cpp` && `evaluation.cpp`
部分修改见 `shared.hpp`, 将 SharedPtr 改为线程安全版本。

---

开启多线程优化的全局编译控制符为 PARALLEL_OPTIMIZE

细分控制符还有 
PARALLEL_OPTIMIZE_PARSE
PARALLEL_OPTIMIZE_EVAL

---
# Extension 2 惰性求值优化设计说明

1. 增加 LazyEval : ValueBase 类，存储 Expr expr 和 Env env
2. 在 evaluation 阶段，遇到需要添加变量时，不直接求值，而是放入 LazyValue
3. 对于 Var::eval(e)，获取到 x 对应的 v 时，若 v 类型为 LazyValue，则根据 v 的 expr 和 env 求值，并 modify 当前 x 在目前 e 内的值(Step 3 的做法实现了记忆化)

具体实现主要见 evaluation.cpp

---
开启惰性求值的全局编译控制符为 LAZYEVAL_OPTIMIZE

--LAZYEVAL_OPTIMIZE 和 PARALLEL_OPTIMIZE_EVAL 冲突，不能同时开启(见 Def.hpp static_assert)--

好像冲突关系也不大，开就开吧...


