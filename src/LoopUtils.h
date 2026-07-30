// LoopUtils.h
#ifndef LOOP_UTILS_H
#define LOOP_UTILS_H

#include <functional>
#include <clang/AST/Stmt.h>
#include <clang/AST/Decl.h>
#include <clang/AST/ASTContext.h>

// Finds the first directly-nested ForStmt inside a body (handles both a
// braced CompoundStmt containing the inner loop, and a braceless single
// statement that IS the inner loop directly).
clang::ForStmt* findDirectlyNestedFor(clang::Stmt *S);

// Extracts the loop's induction VarDecl from `for (VAR = ...; ...; ...)`
// or `for (TYPE VAR = ...; ...; ...)`. Returns nullptr if it can't
// confidently identify one (e.g. `for (;;)` or multi-decl init).
clang::VarDecl* getInductionVar(clang::ForStmt *FS);

// Caller-supplied trip-count function, so LoopUtils doesn't need to
// depend on ASTFeatureExtractor (which owns getTripCount).
using TripCountFn = std::function<long long(clang::ForStmt*, clang::ASTContext*)>;

// Sum of iteration counts over ALL direct-child for-loops of `Body`,
// not just the first one found -- e.g. gemm's i-loop body has a
// sibling j-loop AND a sibling k-loop (which itself nests a j-loop);
// both run once per outer iteration, so their spaces ADD, and each is
// itself expanded through its own nested chain (multiplied down).
// Returns 0 if Body has no direct nested for-loop (a leaf body), or
// -1 if any contributing loop's trip count isn't statically known.
long long sumNestedIterationSpace(clang::Stmt *Body, clang::ASTContext *Context, const TripCountFn &getTripCount);

#endif