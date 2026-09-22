; ModuleID = 'extracted/capc_region_0.c'
source_filename = "extracted/capc_region_0.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-conda-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: write) uwtable
define dso_local void @capc_region_0(ptr noalias noundef writeonly captures(none) %0, ptr noalias noundef writeonly captures(none) %1) local_unnamed_addr #0 {
  br label %3

3:                                                ; preds = %2, %27
  %4 = phi i64 [ 0, %2 ], [ %28, %27 ]
  %5 = add nuw nsw i64 %4, 17
  %6 = getelementptr inbounds nuw [17 x [17 x float]], ptr %1, i64 %4
  %7 = getelementptr inbounds nuw [17 x [17 x float]], ptr %0, i64 %4
  br label %8

8:                                                ; preds = %3, %24
  %9 = phi i64 [ 0, %3 ], [ %25, %24 ]
  %10 = add nuw nsw i64 %5, %9
  %11 = getelementptr inbounds nuw [17 x float], ptr %6, i64 %9
  %12 = getelementptr inbounds nuw [17 x float], ptr %7, i64 %9
  br label %13

13:                                               ; preds = %8, %13
  %14 = phi i64 [ 0, %8 ], [ %22, %13 ]
  %15 = sub nuw nsw i64 %10, %14
  %16 = trunc nuw nsw i64 %15 to i32
  %17 = sitofp i32 %16 to float
  %18 = fmul contract float %17, 1.000000e+01
  %19 = fdiv contract float %18, 1.700000e+01
  %20 = getelementptr inbounds nuw float, ptr %11, i64 %14
  store float %19, ptr %20, align 4, !tbaa !9
  %21 = getelementptr inbounds nuw float, ptr %12, i64 %14
  store float %19, ptr %21, align 4, !tbaa !9
  %22 = add nuw nsw i64 %14, 1
  %23 = icmp eq i64 %22, 17
  br i1 %23, label %24, label %13, !llvm.loop !11

24:                                               ; preds = %13
  %25 = add nuw nsw i64 %9, 1
  %26 = icmp eq i64 %25, 17
  br i1 %26, label %27, label %8, !llvm.loop !14

27:                                               ; preds = %24
  %28 = add nuw nsw i64 %4, 1
  %29 = icmp eq i64 %28, 17
  br i1 %29, label %30, label %3, !llvm.loop !15

30:                                               ; preds = %27
  ret void
}

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: write) uwtable "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

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
!10 = !{!"float", !7, i64 0}
!11 = distinct !{!11, !12, !13}
!12 = !{!"llvm.loop.mustprogress"}
!13 = !{!"llvm.loop.unroll.disable"}
!14 = distinct !{!14, !12, !13}
!15 = distinct !{!15, !12, !13}
