#include <cstring>
#include <map>
#include <vector>

#include "Def.hpp"
#include "RE.hpp"
#include "expr.hpp"
#include "syntax.hpp"
#include "value.hpp"

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
}

// End of added function

Value Let::eval(Assoc &env) {
  Assoc env1 = env;
  for (auto& [x, expr] : bind) {
    Assoc env2 = env;
    env1 = extend(x, expr->eval(env2), env1);
  }
  return body->eval(env1);
}

Value Lambda::eval(Assoc &env) {  
  return make_value(new Closure(x, e, env));
}

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
  for (int i = 0; i < n; i++) {
    Assoc e2 = e;
    std::string var_name = proc->parameters[i];
    Value var_v = rand[i]->eval(e2);
    closure_env = extend(var_name, var_v, closure_env);
  }
  // print_Assoc(std::cerr, closure_env);
  return proc->e->eval(closure_env);
}

Value Letrec::eval(Assoc &env) {
  Assoc env1 = env;
  for (auto& [x, expr] : bind)
    env1 = extend(x, Value(nullptr), env1);
  for (auto& [x, expr] : bind)
    modify(x, expr->eval(env1), env1);
  // ... 不能改，不然闭包是错的，可能被舍弃了一些信息，modify 是好的。 TESTCASE 117
  // for (auto& [x, expr] : bind) {
  //   Value v = find(x, env1);
  //   if (v->v_type == V_PROC) {
  //     Closure *proc = dynamic_cast<Closure*>(v.get());
  //     proc->env = env1; //? 好像不用改，因为用的是 modify
  //   }
  // }
  return body->eval(env1);
}

Value Var::eval(Assoc &e) {
  Value x_v = find(x, e);
  if (x_v.get() == nullptr)
    throw RuntimeError("Runtime Error: varible not found");
  return x_v;
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
  Value result(nullptr);
  for (auto& expr : es) {
    Assoc e1 = e;
    result = expr->eval(e1);
  }
  return result;
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
  if (Identifier *idf = dynamic_cast<Identifier*>(stx.get()))
    return make_value(new Symbol(idf->s));
  Assoc env1 = env, env2 = env;
  return stx->parse(env1)->eval(env2); // Fixnum or Boolean
}
Value quote_construct_list(const std::vector<Syntax>& stxs, int at, Assoc& env) {
  int n = stxs.size();
  if (n == 0 || at == n) // 不管怎么样最后都要赛一个 Null
    return make_value(new Null());
  return make_value(new Pair(quote_syntax_interpret(stxs[at], env), quote_construct_list(stxs, at + 1, env)));
}

Value Quote::eval(Assoc &e) {
  Assoc e1 = e;
  if (List* list = dynamic_cast<List*>(s.get()))
    return quote_construct_list(list->stxs, 0, e1);
  return quote_syntax_interpret(s, e1);
}

Value MakeVoid::eval(Assoc &e) {
  return make_value(new Void());
}

Value Exit::eval(Assoc &e) {
  return make_value(new Terminate());
}

Value Binary::eval(Assoc &e) { // 吐槽一下，为什么不直接一路 Eval 继承到底呢，而要搞出个 evalRator
  Assoc e1 = e, e2 = e;
  Value v1 = rand1->eval(e1), v2 = rand2->eval(e2);
  return evalRator(v1, v2);
}


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
  } else
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