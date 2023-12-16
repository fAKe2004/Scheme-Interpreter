#ifndef PARALLEL_H
#define PARALLEL_H 1

#include "Def.hpp"

#ifdef PARALLEL_OPTIMIZE

#include <thread>
#include <atomic>

#include "syntax.hpp"
#include "expr.hpp"
#include "value.hpp"
#include "RE.hpp"

void parse_mt(Syntax& stx, Assoc env /* copy */, Expr& result, std::atomic<bool>& exception_flag);

void eval_mt(Expr& expr, Assoc env /* copy */, Value& result, std::atomic<bool>& exception_flag);

// Parse: launch parallel parse processes for stxs
std::vector<Expr> parse_mt_launch(std::vector<Syntax>& stxs, const Assoc& env);

// Eval: launch parallel evaluation processes for exprs
std::vector<Value> eval_mt_launch(std::vector<Expr>& exprs, const Assoc& env);
#endif
#endif