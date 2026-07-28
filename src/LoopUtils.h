// LoopUtils.h
#ifndef LOOP_UTILS_H
#define LOOP_UTILS_H

#include <clang/AST/Stmt.h>
#include <clang/AST/Decl.h>

// Finds the first directly-nested ForStmt inside a body (handles both a
// braced CompoundStmt containing the inner loop, and a braceless single
// statement that IS the inner loop directly).
clang::ForStmt* findDirectlyNestedFor(clang::Stmt *S);

// Extracts the loop's induction VarDecl from `for (VAR = ...; ...; ...)`
// or `for (TYPE VAR = ...; ...; ...)`. Returns nullptr if it can't
// confidently identify one (e.g. `for (;;)` or multi-decl init).
clang::VarDecl* getInductionVar(clang::ForStmt *FS);

#endif