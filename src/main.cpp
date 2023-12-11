#include <iostream>
#include <map>
#include <sstream>

#include "Def.hpp"
#include "RE.hpp"
#include "expr.hpp"
#include "syntax.hpp"
#include "value.hpp"

extern std::map<std::string, ExprType> primitives;
extern std::map<std::string, ExprType> reserved_words;

void REPL() {
  // read - evaluation - print loop
  while (1) {
    printf("scm> ");
    Syntax stx = readSyntax(std::cin);  // read
    try {
#ifdef DEBUG_FLAG
      stx->show(std::cerr); // syntax print
      std::cerr << std::endl;
#endif
      Assoc global_env = empty();
      Expr expr = stx->parse(global_env);  // parse
      global_env = empty(); // 微调了下这个，避免 input = s 输出 env_placeholder 的问题
      Value val = expr->eval(global_env);
      if (val->v_type == V_TERMINATE) break;
#ifdef DEBUG_FLAG
      std::cerr << "\noutput> ";
#endif
      val->show(std::cout);  // value print
    } catch (const RuntimeError &RE) {
#ifdef DEBUG_FLAG   
      std::cout << RE.message();
#else
      std::cout << "RuntimeError";
#endif
    }
    puts("");
  }
}

int main(int argc, char *argv[]) {
  initPrimitives();
  initReservedWords();
  REPL();
  return 0;
}

/*
cmake --build build --target myscheme

(letrec ((F (lambda (func-arg)
              (lambda (n) (if (= n 0) 1 (* n (func-arg (- n 1)))))))) (F 10))

./score.sh > test.out 2>&1
*/