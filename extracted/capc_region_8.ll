; ModuleID = 'extracted/capc_region_8.c'
source_filename = "extracted/capc_region_8.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-conda-linux-gnu"

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local void @capc_region_8(ptr noalias noundef readnone captures(none) %0, ptr noalias noundef readnone captures(none) %1, ptr noalias noundef readnone captures(none) %2, ptr noalias noundef readnone captures(none) %3, ptr noalias noundef readnone captures(none) %4, double noundef %5, ptr noalias noundef readnone captures(none) %6, ptr noalias noundef readnone captures(none) %7, ptr noalias noundef readnone captures(none) %8, ptr noalias noundef readnone captures(none) %9) local_unnamed_addr #0 {
  ret void
}

attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}
!llvm.errno.tbaa = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"clang version 22.1.5 (https://github.com/conda-forge/clangdev-feedstock 1176667501d86e025ce26346f5455a62690605e4)"}
!5 = !{!6, !6, i64 0}
!6 = !{!"int", !7, i64 0}
!7 = !{!"omnipotent char", !8, i64 0}
!8 = !{!"Simple C/C++ TBAA"}
