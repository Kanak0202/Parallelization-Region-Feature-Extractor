#include <stddef.h>

#define N 50

/* =====================================================================
   REGION 0: Baseline - pure bookkeeping increment (regression control)
   Expect: Int Arithmetic = 0
   The induction var's increment only feeds the phi + icmp + GEP index.
   This must NOT regress after the fix - it's the case the original
   (unconditional) exclusion logic was designed for.
   ===================================================================== */
void baseline_pure_bookkeeping(double b[N][N]) {
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            b[i][j] = 0.0;
    #pragma capc profitability_region end
}

/* =====================================================================
   REGION 1: Double-duty add (the original bug) - outer index escapes
   Expect: Int Arithmetic >= 1
   %inc = add i, 1 feeds BOTH the phi back-edge AND the stored value
   (via trunc/uitofp). This is the case that motivated the fix.
   ===================================================================== */
void doubleduty_outer(double a[N][N]) {
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            a[i][j] = i + 1;
    #pragma capc profitability_region end
}

/* =====================================================================
   REGION 2: Double-duty on the INNER induction var instead of outer.
   Expect: Int Arithmetic >= 1
   Confirms the fix isn't accidentally scoped to only the outer phi.
   ===================================================================== */
void doubleduty_inner(double a[N][N]) {
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            a[i][j] = j + 1;
    #pragma capc profitability_region end
}

/* =====================================================================
   REGION 3: Double-duty with SUBTRACT instead of add.
   Expect: Int Arithmetic >= 1
   Checks the fix isn't accidentally keyed to the add opcode - the
   users() check should be opcode-agnostic.
   ===================================================================== */
void doubleduty_sub(double a[N][N]) {
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            a[i][j] = N - i;
    #pragma capc profitability_region end
}

/* =====================================================================
   REGION 4: Double-duty with MULTIPLY escaping use.
   Expect: Int Arithmetic >= 1 (and Int Multiply may also fire)
   The escaping use itself is arithmetic (not just a cast chain like
   trunc/uitofp) - checks the "any non-loop-control use" branch, not
   just the trunc+uitofp shape seen in the original bug report.
   ===================================================================== */
void doubleduty_escapes_into_mul(double a[N][N]) {
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            a[i][j] = (double)(i * 2);
    #pragma capc profitability_region end
}

/* =====================================================================
   REGION 5: Induction var used in comparison TWICE (two exit checks
   against the same incoming value, e.g. an early-exit style loop
   lowered with an extra icmp). Expect: Int Arithmetic = 0
   Stresses "allUsesAreLoopControl" when there are >1 ICmpInst users,
   not just the single loop-exit icmp assumed in the original fix.
   ===================================================================== */
void multiple_icmp_users(double a[N][N], int limit) {
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++) {
        if (i >= limit) break;   // extra icmp on i's incoming value
        for (int j = 0; j < N; j++)
            a[i][j] = 0.0;
    }
    #pragma capc profitability_region end
}

/* =====================================================================
   REGION 6: Non-unit stride increment that ALSO escapes as arithmetic.
   Expect: Int Arithmetic >= 1
   i += 2 both drives the loop and is stored - checks the fix handles
   non-1 step values, not just "+1" like the bug report's example.
   ===================================================================== */
void doubleduty_nonunit_stride(double a[N][N]) {
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i += 2)
        for (int j = 0; j < N; j++)
            a[i][j] = (double)i;
    #pragma capc profitability_region end
}

/* =====================================================================
   REGION 7: Decrementing (downward) loop, double-duty.
   Expect: Int Arithmetic >= 1
   Checks the fix isn't assuming an increasing induction variable.
   ===================================================================== */
void doubleduty_downward(double a[N][N]) {
    #pragma capc profitability_region begin
    for (int i = N - 1; i >= 0; i--)
        for (int j = 0; j < N; j++)
            a[i][j] = (double)i;
    #pragma capc profitability_region end
}

/* =====================================================================
   REGION 8: TWO separate phis in the same loop header - one purely
   drives addressing (GEP, must stay excluded), the other is an
   auxiliary counter with a double-duty escaping use (must NOT be
   excluded). Expect: Int Arithmetic >= 1
   Stresses that the fix's per-phi decision doesn't get confused when
   multiple incoming values are in play in the same loop nest.
   ===================================================================== */
void two_phis_one_escapes(double a[N][N]) {
    #pragma capc profitability_region begin
    int counter = 0;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            a[i][j] = (double)counter;   // counter double-duties as sum tracker
            counter = counter + 1;
        }
    }
    #pragma capc profitability_region end
}

/* =====================================================================
   REGION 9: Induction var increment feeds a FUNCTION CALL, not a
   store/GEP directly. Expect: behavior depends on whether calls count
   as "escaping" - this documents current behavior (call is not a phi
   or ICmpInst, so allUsesAreLoopControl should go false -> counted).
   Function Calls should show 1 (or N calls if inlined-away check fails).
   ===================================================================== */
extern void sink(int x);
void doubleduty_into_call(void) {
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++) {
        sink(i + 1);
    }
    #pragma capc profitability_region end
}

/* =====================================================================
   REGION 10: Triangular / non-rectangular bound (j depends on i).
   Expect: Int Arithmetic = 0 for the pure bookkeeping increments, but
   confirms the bound-computation exclusion (existing logic, lines
   59-69) still fires correctly alongside the new check and they don't
   fight each other.
   ===================================================================== */
void triangular_bound(double a[N][N]) {
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        for (int j = 0; j < i; j++)   // bound depends on i -> BoundInst exclusion path
            a[i][j] = 0.0;
    #pragma capc profitability_region end
}

/* =====================================================================
   REGION 11: Triangular bound WHERE the bound-driving instruction
   ALSO double-duties as stored arithmetic (combines region 10's path
   with region 1's path in a single loop).
   Expect: Int Arithmetic >= 1 for the outer/bound value, since it
   escapes into the store as well as driving the inner bound.
   ===================================================================== */
void triangular_bound_doubleduty(double a[N][N]) {
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < i; j++)
            a[i][j] = (double)i;   // i escapes into store AND drives inner bound
    }
    #pragma capc profitability_region end
}

/* =====================================================================
   REGION 12: Reduction - accumulator phi (not an induction var driving
   a GEP) alongside a separate pure-bookkeeping index phi.
   Expect: Int Arithmetic = 0 (accumulator is float, add is fadd not
   counted as int arith; index increments are pure bookkeeping).
   Sanity check that the fix doesn't misfire on reduction-heavy loops.
   ===================================================================== */
double reduction_only(double a[N]) {
    #pragma capc profitability_region begin
    double sum = 0.0;
    for (int i = 0; i < N; i++)
        sum += a[i];
    return sum;
    #pragma capc profitability_region end
}

/* =====================================================================
   REGION 13: Single-trip-count-relevant edge: N=1 loop bound (may get
   fully unrolled/simplified by LLVM into straight-line code with no
   phi at all). Expect: no crash, Int Arithmetic reflects whatever
   survives after LLVM potentially removes the loop entirely.
   Guards against a null/empty-phis crash in collectInductionVarInstrs.
   ===================================================================== */
void single_trip_loop(double a[1][1]) {
    #pragma capc profitability_region begin
    for (int i = 0; i < 1000; i++)
        for (int j = 0; j < 1000; j++)
            a[i][j] = (double)(i + 1);
    #pragma capc profitability_region end
}

int main(void) {
    static double a[N][N], b[N][N];
    baseline_pure_bookkeeping(b);
    doubleduty_outer(a);
    doubleduty_inner(a);
    doubleduty_sub(a);
    doubleduty_escapes_into_mul(a);
    multiple_icmp_users(a, N);
    doubleduty_nonunit_stride(a);
    doubleduty_downward(a);
    two_phis_one_escapes(a);
    doubleduty_into_call();
    triangular_bound(a);
    triangular_bound_doubleduty(a);
    reduction_only(a[0]);
    single_trip_loop((double(*)[1])a);
    return 0;
}