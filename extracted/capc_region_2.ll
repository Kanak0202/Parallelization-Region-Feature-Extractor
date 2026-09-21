; ModuleID = 'extracted/capc_region_2.c'
source_filename = "extracted/capc_region_2.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-conda-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable
define dso_local void @capc_region_2(ptr noalias noundef captures(none) %0, ptr noalias noundef readonly captures(none) %1, ptr noalias noundef readonly captures(none) %2) local_unnamed_addr #0 {
  br label %4

4:                                                ; preds = %3, %27
  %5 = phi i64 [ 0, %3 ], [ %28, %27 ]
  %6 = getelementptr inbounds nuw [10 x double], ptr %0, i64 %5
  %7 = getelementptr inbounds nuw [10 x double], ptr %1, i64 %5
  br label %8

8:                                                ; preds = %4, %24
  %9 = phi i64 [ 0, %4 ], [ %25, %24 ]
  %10 = getelementptr inbounds nuw double, ptr %6, i64 %9
  %11 = getelementptr inbounds nuw double, ptr %2, i64 %9
  %12 = load double, ptr %10, align 8, !tbaa !9
  br label %13

13:                                               ; preds = %8, %13
  %14 = phi i64 [ 0, %8 ], [ %22, %13 ]
  %15 = phi double [ %12, %8 ], [ %21, %13 ]
  %16 = getelementptr inbounds nuw double, ptr %7, i64 %14
  %17 = load double, ptr %16, align 8, !tbaa !9
  %18 = getelementptr inbounds nuw [10 x double], ptr %11, i64 %14
  %19 = load double, ptr %18, align 8, !tbaa !9
  %20 = fmul contract double %17, %19
  %21 = fadd contract double %15, %20
  %22 = add nuw nsw i64 %14, 1
  %23 = icmp eq i64 %22, 10
  br i1 %23, label %24, label %13, !llvm.loop !11

24:                                               ; preds = %13
  store double %21, ptr %10, align 8, !tbaa !9
  %25 = add nuw nsw i64 %9, 1
  %26 = icmp eq i64 %25, 10
  br i1 %26, label %27, label %8, !llvm.loop !14

27:                                               ; preds = %24
  %28 = add nuw nsw i64 %5, 1
  %29 = icmp eq i64 %28, 10
  br i1 %29, label %30, label %4, !llvm.loop !15

30:                                               ; preds = %27
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
!14 = distinct !{!14, !12, !13}
!15 = distinct !{!15, !12, !13}
