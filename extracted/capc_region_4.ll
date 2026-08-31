; ModuleID = 'extracted/capc_region_4.c'
source_filename = "extracted/capc_region_4.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-conda-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable
define dso_local void @capc_region_4(ptr noalias noundef readnone captures(none) %0, ptr noalias noundef readonly captures(none) %1, ptr noalias noundef readonly captures(none) %2, ptr noalias noundef readonly captures(none) %3, ptr noalias noundef readnone captures(none) %4, ptr noalias noundef readonly captures(none) %5, ptr noalias noundef captures(none) %6, ptr noalias noundef captures(none) %7, ptr noalias noundef captures(none) %8, ptr noalias noundef readonly captures(none) %9) local_unnamed_addr #0 {
  %11 = load double, ptr %6, align 8, !tbaa !9
  %12 = load double, ptr %7, align 8, !tbaa !9
  %13 = load double, ptr %8, align 8, !tbaa !9
  br label %14

14:                                               ; preds = %10, %14
  %15 = phi i64 [ 0, %10 ], [ %38, %14 ]
  %16 = phi double [ %11, %10 ], [ %30, %14 ]
  %17 = phi double [ %12, %10 ], [ %34, %14 ]
  %18 = phi double [ %13, %10 ], [ %37, %14 ]
  %19 = getelementptr inbounds nuw double, ptr %1, i64 %15
  %20 = load double, ptr %19, align 8, !tbaa !9
  %21 = fmul contract double %20, 5.000000e-01
  %22 = getelementptr inbounds nuw double, ptr %2, i64 %15
  %23 = load double, ptr %22, align 8, !tbaa !9
  %24 = fmul contract double %23, %23
  %25 = getelementptr inbounds nuw double, ptr %3, i64 %15
  %26 = load double, ptr %25, align 8, !tbaa !9
  %27 = fmul contract double %26, %26
  %28 = fadd contract double %24, %27
  %29 = fmul contract double %21, %28
  %30 = fadd contract double %16, %29
  %31 = getelementptr inbounds nuw double, ptr %5, i64 %15
  %32 = load double, ptr %31, align 8, !tbaa !9
  %33 = fmul contract double %20, %32
  %34 = fadd contract double %17, %33
  %35 = getelementptr inbounds nuw double, ptr %9, i64 %15
  %36 = load double, ptr %35, align 8, !tbaa !9
  %37 = fadd contract double %36, %18
  %38 = add nuw nsw i64 %15, 1
  %39 = icmp eq i64 %38, 2558
  br i1 %39, label %40, label %14, !llvm.loop !11

40:                                               ; preds = %14
  store double %30, ptr %6, align 8, !tbaa !9
  store double %34, ptr %7, align 8, !tbaa !9
  store double %37, ptr %8, align 8, !tbaa !9
  ret void
}

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

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
!9 = !{!10, !10, i64 0}
!10 = !{!"double", !7, i64 0}
!11 = distinct !{!11, !12, !13}
!12 = !{!"llvm.loop.mustprogress"}
!13 = !{!"llvm.loop.unroll.disable"}
