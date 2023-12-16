// parallel.hpp/cpp
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