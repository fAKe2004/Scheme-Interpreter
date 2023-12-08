#ifndef PARSER
#define PARSER

// parser of myscheme
// 吐槽一下，parse 阶段的 env 单纯是为了“关键字可以变成变量名”服务的...


#include <cstring>
#include <iostream>
#include <map>

#include "Def.hpp"
#include "RE.hpp"
#include "expr.hpp"
#include "syntax.hpp"

// ADDED
#include <functional>

#include "value.hpp"

#define mp make_pair
using std::pair;
using std::string;
using std::vector;

extern std::map<std ::string, ExprType> primitives;
extern std::map<std ::string, ExprType> reserved_words;

// My Functions

// return a Expr object from a pointer to derived classes of Expr
template <class Derived_Expr>
Expr make_expr(Derived_Expr *ptr) {
  return Expr(static_cast<ExprBase *>(ptr));
}

bool var_name_check(const std::string &s) {
  static std::function<bool(char)> is_varchar = [](char ch) -> bool {
    // return isalnum(ch) || ch == '_' || ch == '.' || ch == '-';
    return true; // 。。。什么奇奇怪怪的变量名啊，不判了，全 true 吧。
  };
  if (s.empty()) return false;
  if (isdigit(s[0])) return false;
  for (auto ch : s)
    if (!is_varchar(ch)) return false;
  return true;
}

// try to interpret as varible as long as possible
// throw if failed
void var_cast(const Syntax &stx, Assoc &env) {
  Identifier *stx_ptr = dynamic_cast<Identifier *>(stx.get());
  if (stx_ptr == nullptr || !var_name_check(stx_ptr->s))
    throw RuntimeError("Syntax Error: invaild varible");
  env = extend(stx_ptr->s, IntegerV(1), env);
  // return make_expr(new Var(stx_ptr->s));
  return void();
}

// END OF My Functions

Expr Syntax::parse(Assoc &env) { return ptr->parse(env); }

Expr Number::parse(Assoc &env) { return make_expr(new Fixnum(this->n)); }

Expr Identifier::parse(Assoc &env) {
  if (s == "#t")
    return make_expr(new True());
  if (s == "#f")
    return make_expr(new False());

  if (find(s, env).get() != nullptr)  // existing var .. 好像不能这么判断，咋整，因为找不到和没赋值的变量都是 nullptr 规定存在但为求值得变量为 Int 1 吧，反正 parse 阶段 env 值也不重要
    return make_expr(new Var(s));

  if (primitives.find(s) != primitives.end())  // primitives
    return Expr(new ExprBase(primitives[s]));  // abstract, only to return type information
                                               // info. handle to List::parse.

  if (reserved_words.find(s) != reserved_words.end())  // reserved words
    return Expr(new ExprBase(reserved_words[s]));
  
  if (!var_name_check(s))
    throw RuntimeError("Syntax Error: invalid varible");

  env = extend(s, IntegerV(1), env);  // new var
  return make_expr(new Var(s));
}

Expr TrueSyntax::parse(Assoc &env) { return make_expr(new True()); }

Expr FalseSyntax::parse(Assoc &env) { return make_expr(new False()); }

Expr List::parse(Assoc &env) {
  if (stxs.empty())
    return Expr(static_cast<ExprBase *>(new Quote(new List()))); // equ Null
  Assoc env0 = env;
  Expr front = stxs.front().parse(env0);
  int n = stxs.size();

  switch (front->e_type) { 
    /*
    有一个小问题，这里判断 front 的 type 的时候，需要判断 front 是不是真的是抽象类 ExprBase。
  
    比如 ((lambda (a b) a) 1 2)
    外层 list 的第一个还是 lambda，但是整个式子应该被解释称 function call

    解决方法是... 给 lambda, let, letrec, if, begin 判断一下是不是 ExprBase 类。
    如果是，说明这一列真的是要解析的，不然就是 function call
    */
    
    // Primitives

    case E_MUL: {  // Remark: Expr 类型检查留给 EVAL 吧，好写点。
      if (n != 3) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      Assoc env1 = env0, env2 = env0;
      Expr first = stxs[1].parse(env1), second = stxs[2].parse(env2);
      return make_expr(new Mult(first, second));
    }

    case E_MINUS: {
      if (n != 3) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      Assoc env1 = env0, env2 = env0;
      Expr first = stxs[1].parse(env1), second = stxs[2].parse(env2);
      return make_expr(new Minus(first, second));
    }

    case E_PLUS: {
      if (n != 3) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      Assoc env1 = env0, env2 = env0;
      Expr first = stxs[1].parse(env1), second = stxs[2].parse(env2);
      return make_expr(new Plus(first, second));
    }

    case E_LT: {
      if (n != 3) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      Assoc env1 = env0, env2 = env0;
      Expr first = stxs[1].parse(env1), second = stxs[2].parse(env2);
      return make_expr(new Less(first, second));
    }

    case E_LE: {
      if (n != 3) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      Assoc env1 = env0, env2 = env0;
      Expr first = stxs[1].parse(env1), second = stxs[2].parse(env2);
      return make_expr(new LessEq(first, second));
    }

    case E_EQ: {
      if (n != 3) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      Assoc env1 = env0, env2 = env0;
      Expr first = stxs[1].parse(env1), second = stxs[2].parse(env2);
      return make_expr(new Equal(first, second));
    }

    case E_GE: {
      if (n != 3) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      Assoc env1 = env0, env2 = env0;
      Expr first = stxs[1].parse(env1), second = stxs[2].parse(env2);
      return make_expr(new GreaterEq(first, second));
    }

    case E_GT: {
      if (n != 3) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      Assoc env1 = env0, env2 = env0;
      Expr first = stxs[1].parse(env1), second = stxs[2].parse(env2);
      return make_expr(new Greater(first, second));
    }

    case E_VOID: {
      if (n != 1) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      return make_expr(new MakeVoid());
    }

    case E_EQQ: {
      if (n != 3) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      Assoc env1 = env0, env2 = env0;
      Expr first = stxs[1].parse(env1), second = stxs[2].parse(env2);
      return make_expr(new IsEq(first, second));
    }

    case E_BOOLQ: {
      if (n != 2) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      Assoc env1 = env0;
      Expr expr = stxs[1].parse(env1);
      return make_expr(new IsBoolean(expr));
    }

    case E_INTQ: {
      if (n != 2) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      Assoc env1 = env0;
      Expr expr = stxs[1].parse(env1);
      return make_expr(new IsFixnum(expr));
    }

    case E_NULLQ: {
      if (n != 2) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      Assoc env1 = env0;
      Expr expr = stxs[1].parse(env1);
      return make_expr(new IsNull(expr));
    }

    case E_PAIRQ: {
      if (n != 2) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      Assoc env1 = env0;
      Expr expr = stxs[1].parse(env1);
      return make_expr(new IsPair(expr));
    }

    case E_PROCQ: {
      if (n != 2) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      Assoc env1 = env0;
      Expr expr = stxs[1].parse(env1);
      return make_expr(new IsProcedure(expr));
    }

    case E_CONS: {
      if (n != 3) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      Assoc env1 = env0, env2 = env0;
      Expr first = stxs[1].parse(env1), second = stxs[2].parse(env2);
      return make_expr(new Cons(first, second));
    }

    case E_NOT: {
      if (n != 2) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      Assoc env1 = env0;
      Expr expr = stxs[1].parse(env1);
      return make_expr(new Not(expr));
    }

    case E_CAR: {
      if (n != 2) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      Assoc env1 = env0;
      Expr expr = stxs[1].parse(env1);
      return make_expr(new Car(expr));
    }

    case E_CDR: {
      if (n != 2) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      Assoc env1 = env0;
      Expr expr = stxs[1].parse(env1);
      return make_expr(new Cdr(expr));
    }

    case E_EXIT: {
      if (n != 1) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      return make_expr(new Exit());
    }

    // Reserved Words // Hard Part
    case E_LET: {
      if (dynamic_cast<Let*>(front.get()) != nullptr)
        break; // function call, not real let

      if (n != 3) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      
      List* var_list = dynamic_cast<List*>(stxs[1].get());
      if (var_list == nullptr)
        throw RuntimeError("Syntax Error: let (var*) is not a List ");

      std::vector<std::pair<std::string, Expr>> bind;
      Assoc env1 = env0;
      for (auto& stx : var_list->stxs) { // parse var*
        List *xy = dynamic_cast<List*>(stx.get());
        if (xy == nullptr || xy->stxs.size() != 2)
          throw RuntimeError("Syntax Error: let var not in form of [x y]");
        Syntax var_stx = xy->stxs[0], expr_stx = xy->stxs[1];
        var_cast(var_stx, env1); // var_cast guarantees var_stx is a valid var identifier
        std::string var_name = dynamic_cast<Identifier*>(var_stx.get())->s;
        Assoc env2 = env0;
        bind.emplace_back(var_name, expr_stx.parse(env2));
      }
      
      Expr body = stxs[2]->parse(env1);

      return make_expr(new Let(bind, body));
    }

    case E_LAMBDA: {
      if (dynamic_cast<Lambda*>(front.get()) != nullptr)
        break;

      if (n != 3) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      
      List* var_list = dynamic_cast<List*>(stxs[1].get());
      if (var_list == nullptr)
        throw RuntimeError("Syntax Error: lambda (var*) is not a List");
      
      std::vector<std::string> x;
      Assoc env1 = env0;
      for (auto& var_stx : var_list->stxs) {
        var_cast(var_stx, env1);// var_cast guarantees var_stx is a valid var identifier
        std::string var_name = dynamic_cast<Identifier*>(var_stx.get())->s;
        x.emplace_back(var_name);
      }
      
      Expr body = stxs[2]->parse(env1);
      return make_expr(new Lambda(x, body));
    }

    case E_LETREC: { 
      if (dynamic_cast<Letrec*>(front.get()) != nullptr)
        break;

      // 把获取变量和变量对应的 expr 解析分开就行
      if (n != 3) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      
      List* var_list = dynamic_cast<List*>(stxs[1].get());
      if (var_list == nullptr)
        throw RuntimeError("Syntax Error: letrec (var*) is not a List");

      std::vector<std::pair<std::string, Expr>> bind;
      Assoc env1 = env0;
      for (auto& stx : var_list->stxs) { // parse var* | add var to env only
        List *xy = dynamic_cast<List*>(stx.get());
        if (xy == nullptr || xy->stxs.size() != 2)
          throw RuntimeError("Syntax Error: let var not in form of [x y]");
        Syntax var_stx = xy->stxs[0];
        var_cast(var_stx, env1); // var_cast guarantees var_stx is a valid var identifier
      }

      for (auto& stx : var_list->stxs) { // parse var* | parse expr only
        List *xy = dynamic_cast<List*>(stx.get());
        Syntax var_stx = xy->stxs[0], expr_stx = xy->stxs[1];
        std::string var_name = dynamic_cast<Identifier*>(var_stx.get())->s;
        Assoc env2 = env1;
        bind.emplace_back(var_name, expr_stx.parse(env2));
      }
      
      Expr body = stxs[2]->parse(env1);
      return make_expr(new Letrec(bind, body));
    }

    case E_IF: {
      if (dynamic_cast<If*>(front.get()) != nullptr)
        break;

      if (n != 4) throw RuntimeError("Syntax Error: List::parse wrong number of args");
      Assoc env1 = env0, env2 = env0, env3 = env0;
      Syntax cond_stx = stxs[1], conseq_stx = stxs[2], alter_stx = stxs[3];
      Expr cond = cond_stx.parse(env1), conseq = conseq_stx.parse(env2), alter = alter_stx.parse(env3);
      return make_expr(new If(cond, conseq, alter));
    }

    case E_BEGIN: {
      if (dynamic_cast<Begin*>(front.get()) != nullptr)
        break;

      if (n < 2) throw RuntimeError("Syntax Error: List::parse wrong number of args");

      std::vector<Expr> es;
      for (int i = 1; i < n; i++) {
        Syntax& stx = stxs[i];
        Assoc env1 = env0;
        es.push_back(stx.parse(env1));
      }

      return make_expr(new Begin(es));
    }

    case E_QUOTE: {
      if (n != 2) throw RuntimeError("Syntax Error: List::parse wrong number of args");

      Syntax stx = stxs[1];
      return make_expr(new Quote(stx));
    }

    default: { 
      
    }
  }

  // if (n == 1) // 这个 n = 1 的判断要移动下来，因为可能 first parse 返回的是抽象类，包含 void 要被 switch 解析，不能放在 switch 前面
    // return front;
  
  // 还是不能这么判...有接受 0 个参数求值的情况，要不全部解释成 Apply 去 Apply 里面判断是不是常量？
  // 没事了... 只要在一般 List 里面就默认解释位 function call，比如 (1) 是不合法的。

  // interpret as procedure call, leave type check for later
  Expr rator = front;
  std::vector<Expr> rand;
  for (int i = 1; i < n; i++) {
    Assoc env1 = env0;
    rand.push_back(stxs[i]->parse(env1));
  }
  return make_expr(new Apply(rator, rand));
}

#endif