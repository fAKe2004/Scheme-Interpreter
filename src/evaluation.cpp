#include <cstring>
#include <map>
#include <vector>

#include "Def.hpp"
#include "RE.hpp"
#include "expr.hpp"
#include "syntax.hpp"
#include "value.hpp"

#include "parallel.hpp"

extern std::map<std::string, ExprType> primitives;
extern std::map<std::string, ExprType> reserved_words;


// Added function

bool is_true_judge(const Value& v) {
  if (v->v_type != V_BOOL)
    return true;
  Boolean *v_bool = dynamic_cast<Boolean*>(v.get());
  return v_bool->b == true;
}

template<class Drived_Value>
Value make_value(Drived_Value *ptr) {
  return Value(static_cast<ValueBase*>(ptr));
}

Value ExprBase::eval(Assoc &env) {
  throw RuntimeError("Internal Error: try to evaluate an ExprBase");
  // 也有可能 input = let 这样的只有一个关键字但不在 () 内的情况...严格来说不一定 Internal Error... 反正确实是 RuntimeError，之前还没考虑到只有一个 Identifier 不在 List 内的情况, 歪打正着了……
}

// End of added function

#ifndef PARALLEL_OPTIMIZE_EVAL
Value Let::eval(Assoc &env) {
  Assoc env1 = env;
  for (auto& [x, expr] : bind) {
#ifndef LAZYEVAL_OPTIMIZE
    Assoc env2 = env;
    env1 = extend(x, expr->eval(env2), env1);
#else
    env1 = extend(x, make_value(new LazyEval(expr, env)), env1);
#endif
  }
  return body->eval(env1);
}
#else
Value Let::eval(Assoc &env) {
  int n = bind.size();
  std::vector<Expr> exprs(n, Expr(nullptr));
  for (int i = 0; i < n; i++)
    exprs[i] = bind[i].second;
  std::vector<Value> values = eval_mt_launch(exprs, env);
  Assoc env1 = env;
  for (int i = 0; i < n; i++)
    env1 = extend(bind[i].first, values[i], env1);
  return body->eval(env1);
}
#endif

Value Lambda::eval(Assoc &env) {  
  return make_value(new Closure(x, e, env));
}

#ifndef PARALLEL_OPTIMIZE_EVAL
Value Apply::eval(Assoc &e) {
  Assoc e1 = e;
  Value rator_v = rator->eval(e1);
  if (rator_v->v_type != V_PROC) {
    throw RuntimeError("Runtime Error : attempt to apply an uncallable object");
  }
  Closure *proc = dynamic_cast<Closure*>(rator_v.get());
  if (proc->parameters.size() != rand.size())
    throw RuntimeError("Runtime Error : unmatched parameter number");

  int n = rand.size();
  Assoc closure_env = proc->env;
  // ? 应该把 closure_env 和 外部 env 合并起来？
  for (int i = 0; i < n; i++) {
#ifndef LAZYEVAL_OPTIMIZE
    Assoc e2 = e;
    std::string var_name = proc->parameters[i];
    Value var_v = rand[i]->eval(e2);
    closure_env = extend(var_name, var_v, closure_env);
#else 
    std::string var_name = proc->parameters[i];
    Value var_v = make_value(new LazyEval(rand[i], e));
    closure_env = extend(var_name, var_v, closure_env);
#endif
  }
    return proc->e->eval(closure_env);
}
#else
Value Apply::eval(Assoc &e) {
  Assoc e1 = e;
  Value rator_v = rator->eval(e1);
  if (rator_v->v_type != V_PROC) {
    throw RuntimeError("Runtime Error : attempt to apply an uncallable object");
  }
  Closure *proc = dynamic_cast<Closure*>(rator_v.get());
  if (proc->parameters.size() != rand.size())
    throw RuntimeError("Runtime Error : unmatched parameter number");

  int n = rand.size();
  Assoc closure_env = proc->env;

  std::vector<Value> values = eval_mt_launch(rand, e);
  for (int i = 0; i < n; i++) {
    std::string var_name = proc->parameters[i];
    closure_env = extend(var_name, values[i], closure_env);
  }
  
  return proc->e->eval(closure_env);
}
#endif


#ifndef PARALLEL_OPTIMIZE_EVAL

Value Letrec::eval(Assoc &env) {
  Assoc env1 = env;
  for (auto& [x, expr] : bind)
    env1 = extend(x, Value(nullptr), env1);
  std::vector<std::pair<std::string, Value>> values_bind;
  for (auto& [x, expr] : bind) {
    Assoc env2 = env1;
// #ifndef LAZYEVAL_OPTIMIZE
// 爬吧，letrec 不允许 lazy eval  不然有一大堆问题 删了
    values_bind.push_back(std::make_pair(x, expr->eval(env2)));
// #else
//     values.push_back(std::make_pair(x, make_value(new LazyEval(expr, env2))));
// #endif 
  }
  for (auto& [x, value] : values_bind)
    modify(x, value, env1);
  // ... 下方不能改，不然闭包是错的，可能被舍弃了一些信息，modify 是好的。 TESTCASE 117
  // for (auto& [x, expr] : bind) {
  //   Value v = find(x, env1);
  //   if (v->v_type == V_PROC) {
  //     Closure *proc = dynamic_cast<Closure*>(v.get());
  //     proc->env = env1;
  //   }
  // }
  // print_Assoc(std::cerr, env1);
  return body->eval(env1);
}
#else
Value Letrec::eval(Assoc &env) {
  Assoc env1 = env;
  for (auto& [x, expr] : bind)
    env1 = extend(x, Value(nullptr), env1);
  int n = bind.size();
  std::vector<Expr> exprs(n, Expr(nullptr));
  for (int i = 0; i < n; i++)
    exprs[i] = bind[i].second;
  std::vector<Value> values = eval_mt_launch(exprs, env1);
  for (int i = 0; i < n; i++)
    modify(bind[i].first, values[i], env1);
  return body->eval(env1);
}
#endif


Value Var::eval(Assoc &e) {
  Value x_v = find(x, e);
  if (x_v.get() == nullptr)
    throw RuntimeError("Runtime Error: varible not found");
#ifndef LAZYEVAL_OPTIMIZE
  return x_v;
#else
  if (LazyEval *lz = dynamic_cast<LazyEval*>(x_v.get()))  {
    Value result = lz->eval();
    modify(x, result, e);
    x_v = result;
  }
  return x_v;
#endif
}

Value Fixnum::eval(Assoc &e) {
  return make_value(new Integer(n));
}

Value If::eval(Assoc &e) {
  Assoc e1 = e, e2 = e;
  if (is_true_judge(cond->eval(e1)))
    return conseq->eval(e2);
  else 
    return alter->eval(e2);
}

Value True::eval(Assoc &e) {
  return make_value(new Boolean(true));
}

Value False::eval(Assoc &e) {
  return make_value(new Boolean(false));
}

Value Begin::eval(Assoc &e) {
#ifndef PARALLEL_OPTIMIZE_EVAL
  Value result(nullptr);
  for (auto& expr : es) {
    Assoc e1 = e;
    result = expr->eval(e1);
  }
  return result;
#else
  std::vector<Value> values = eval_mt_launch(es, e);
  return values.back();
#endif
}

// Value syntaxToValue(const Syntax &syntax) {
  // deleted
// }
Value quote_construct_list(const std::vector<Syntax>& stxs, int at, Assoc& env);

Value quote_syntax_interpret(const Syntax& stx, Assoc& env) {
  if (List *ptr = dynamic_cast<List*>(stx.get())) {
    // throw RuntimeError("Syntax Error: invalid quote");
    return quote_construct_list(ptr->stxs, 0, env);
  }
  if (Number *num = dynamic_cast<Number*>(stx.get())) 
    return make_value(new Integer(num->n));
  if (TrueSyntax *ts = dynamic_cast<TrueSyntax*>(stx.get()))
    return make_value(new Boolean(true));
  if (FalseSyntax *fs = dynamic_cast<FalseSyntax*>(stx.get()))
    return make_value(new Boolean(false));
  if (Identifier *idf = dynamic_cast<Identifier*>(stx.get())) {
    return make_value(new Symbol(idf->s));
  }
  throw RuntimeError("Internal Error: quote construction");
}
Value quote_construct_list(const std::vector<Syntax>& stxs, int at, Assoc& env) {
  int n = stxs.size();
  if (n == 0 || at == n) // 不管怎么样最后都要赛一个 Null
    return make_value(new Null());
  return make_value(new Pair(quote_syntax_interpret(stxs[at], env), quote_construct_list(stxs, at + 1, env)));
}

Value Quote::eval(Assoc &e) {
  Assoc e1 = e;
  return quote_syntax_interpret(s, e1);
}

Value MakeVoid::eval(Assoc &e) {
  return make_value(new Void());
}

Value Exit::eval(Assoc &e) {
  return make_value(new Terminate());
}

#ifndef PARALLEL_OPTIMIZE_EVAL
Value Binary::eval(Assoc &e) { // 吐槽一下，为什么不直接一路 Eval 继承到底呢，而要搞出个 evalRator
  Assoc e1 = e, e2 = e;
  Value v1 = rand1->eval(e1), v2 = rand2->eval(e2);
  return evalRator(v1, v2);
}
#else
Value Binary::eval(Assoc &e) {
  std::vector<Expr> exprs({rand1, rand2});
  std::vector<Value> values = eval_mt_launch(exprs, e);
  return evalRator(values[0], values[1]);
}
#endif


Value Unary::eval(Assoc &e) {
  Assoc e1 = e;
  Value v1 = rand->eval(e1);
  return evalRator(v1);
}

Value Mult::evalRator(const Value &rand1, const Value &rand2) {
  if (rand1->v_type != V_INT || rand2->v_type != V_INT)
    throw RuntimeError("Type Error");
  Integer *v1 = dynamic_cast<Integer*>(rand1.get()), 
    *v2 = dynamic_cast<Integer*>(rand2.get());
  return make_value(new Integer(v1->n * v2->n));
}

Value Plus::evalRator(const Value &rand1, const Value &rand2) {
  if (rand1->v_type != V_INT || rand2->v_type != V_INT)
    throw RuntimeError("Type Error");
  Integer *v1 = dynamic_cast<Integer*>(rand1.get()), 
    *v2 = dynamic_cast<Integer*>(rand2.get());
  return make_value(new Integer(v1->n + v2->n));
}

Value Minus::evalRator(const Value &rand1, const Value &rand2) {
  if (rand1->v_type != V_INT || rand2->v_type != V_INT)
    throw RuntimeError("Type Error");
  Integer *v1 = dynamic_cast<Integer*>(rand1.get()), 
    *v2 = dynamic_cast<Integer*>(rand2.get());
  return make_value(new Integer(v1->n - v2->n));
}

Value Less::evalRator(const Value &rand1, const Value &rand2) {
  if (rand1->v_type != V_INT || rand2->v_type != V_INT)
    throw RuntimeError("Type Error");
  Integer *v1 = dynamic_cast<Integer*>(rand1.get()), 
    *v2 = dynamic_cast<Integer*>(rand2.get());
  return make_value(new Boolean(v1->n < v2->n));
}

Value LessEq::evalRator(const Value &rand1, const Value &rand2) {
  if (rand1->v_type != V_INT || rand2->v_type != V_INT)
    throw RuntimeError("Type Error");
  Integer *v1 = dynamic_cast<Integer*>(rand1.get()), 
    *v2 = dynamic_cast<Integer*>(rand2.get());
  return make_value(new Boolean(v1->n <= v2->n));
}

Value Equal::evalRator(const Value &rand1, const Value &rand2) {
  if (rand1->v_type != V_INT || rand2->v_type != V_INT)
    throw RuntimeError("Type Error");
  Integer *v1 = dynamic_cast<Integer*>(rand1.get()), 
    *v2 = dynamic_cast<Integer*>(rand2.get());
  return make_value(new Boolean(v1->n == v2->n));
}

Value GreaterEq::evalRator(const Value &rand1, const Value &rand2) {
  if (rand1->v_type != V_INT || rand2->v_type != V_INT)
    throw RuntimeError("Type Error");
  Integer *v1 = dynamic_cast<Integer*>(rand1.get()), 
    *v2 = dynamic_cast<Integer*>(rand2.get());
  return make_value(new Boolean(v1->n >= v2->n));
}

Value Greater::evalRator(const Value &rand1, const Value &rand2) {
  if (rand1->v_type != V_INT || rand2->v_type != V_INT)
    throw RuntimeError("Type Error");
  Integer *v1 = dynamic_cast<Integer*>(rand1.get()), 
    *v2 = dynamic_cast<Integer*>(rand2.get());
  return make_value(new Boolean(v1->n > v2->n));
}

Value IsEq::evalRator(const Value &rand1, const Value &rand2) {
  if (rand1->v_type == V_INT && rand2->v_type == V_INT) {
    Integer *v1 = dynamic_cast<Integer*>(rand1.get()), 
      *v2 = dynamic_cast<Integer*>(rand2.get());
    return make_value(new Boolean(v1->n == v2->n));
  } else if (rand1->v_type == V_BOOL && rand2->v_type == V_BOOL) {
    Boolean *v1 = dynamic_cast<Boolean*>(rand1.get()), 
      *v2 = dynamic_cast<Boolean*>(rand2.get());
    return make_value(new Boolean(v1->b == v2->b));
  } else if (rand1->v_type == V_SYM && rand2->v_type == V_SYM) {
    Symbol*v1 = dynamic_cast<Symbol*>(rand1.get()), 
    *v2 = dynamic_cast<Symbol*>(rand2.get());  
    return make_value(new Boolean(v1->s == v2->s));
  }
    return make_value(new Boolean(rand1.get() == rand2.get()));
}

Value Cons::evalRator(const Value &rand1, const Value &rand2) {
  return make_value(new Pair(rand1, rand2));
}

Value IsBoolean::evalRator(const Value &rand) {
  return make_value(new Boolean(rand->v_type == V_BOOL));
}

Value IsFixnum::evalRator(const Value &rand) {
  return make_value(new Boolean(rand->v_type == V_INT));
}

Value IsNull::evalRator(const Value &rand) {
  return make_value(new Boolean(rand->v_type == V_NULL));
}

Value IsPair::evalRator(const Value &rand) {
  return make_value(new Boolean(rand->v_type == V_PAIR));
}

Value IsProcedure::evalRator(const Value &rand) {
  return make_value(new Boolean(rand->v_type == V_PROC));

}

// ADDED SYMBOLQ 
Value IsSymbol::evalRator(const Value &rand) {
  return make_value(new Boolean(rand->v_type == V_SYM));
}

Value Not::evalRator(const Value &rand) {
  return make_value(new Boolean(!is_true_judge(rand)));
}

Value Car::evalRator(const Value &rand) {
  if (rand->v_type != V_PAIR)
    throw RuntimeError("Type Error");
  Pair* pair = dynamic_cast<Pair*>(rand.get());
  return pair->car;
}

Value Cdr::evalRator(const Value &rand) {
  if (rand->v_type != V_PAIR)
    throw RuntimeError("Type Error");
  Pair* pair = dynamic_cast<Pair*>(rand.get());
  return pair->cdr;
}