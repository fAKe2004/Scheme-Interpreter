#ifndef PARSER
#define PARSER

// parser of myscheme

#include <cstring>
#include <iostream>
#include <map>

#include "Def.hpp"
#include "RE.hpp"
#include "expr.hpp"
#include "syntax.hpp"
#define mp make_pair
using std ::pair;
using std ::string;
using std ::vector;

extern std ::map<std ::string, ExprType> primitives;
extern std ::map<std ::string, ExprType> reserved_words;

Expr Syntax ::parse(Assoc &env) {}

Expr Number ::parse(Assoc &env) {}

Expr Identifier ::parse(Assoc &env) {}

Expr TrueSyntax ::parse(Assoc &env) {}

Expr FalseSyntax ::parse(Assoc &env) {}

Expr List ::parse(Assoc &env) {}

#endif