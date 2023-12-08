#ifndef DEF_HPP
#define DEF_HPP

// DEBUGE FLAG DEFINITION
// #define DEBUG_FLAG 1

// By luke36

#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

struct Syntax;
struct Expr;
struct Value;
struct AssocList;
struct Assoc;

enum ExprType {
  E_LET,
  E_LAMBDA,
  E_APPLY,
  E_LETREC,
  E_VAR,
  E_FIXNUM,
  E_IF,
  E_TRUE,
  E_FALSE,
  E_BEGIN,
  E_QUOTE,
  E_VOID,
  E_MUL,
  E_PLUS,
  E_MINUS,
  E_LT,
  E_LE,
  E_EQ,
  E_GE,
  E_GT,
  E_CONS,
  E_NOT,
  E_CAR,
  E_CDR,
  E_EQQ,
  E_BOOLQ,
  E_INTQ,
  E_NULLQ,
  E_PAIRQ,
  E_PROCQ,
  E_SYMBOLQ, // ADDED SYMBOLQ
  E_EXIT
};
enum ValueType {
  V_INT,
  V_BOOL,
  V_SYM,
  V_NULL,
  V_STRING,
  V_PAIR,
  V_PROC,
  V_VOID,
  V_PRIMITIVE,
  V_TERMINATE
};

void initPrimitives();
void initReservedWords();

#endif
