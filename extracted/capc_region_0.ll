; ModuleID = 'extracted/capc_region_0.c'
source_filename = "extracted/capc_region_0.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-conda-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: write) uwtable
define dso_local void @capc_region_0(ptr noalias noundef writeonly captures(none) %0, ptr noalias noundef writeonly captures(none) %1, ptr noalias noundef writeonly captures(none) %2, ptr noalias noundef writeonly captures(none) %3, ptr noalias noundef writeonly captures(none) %4, ptr noalias noundef writeonly captures(none) %5, ptr noalias noundef writeonly captures(none) %6) local_unnamed_addr #0 {
  br label %8

8:                                                ; preds = %7, %44
  %9 = phi i64 [ 0, %7 ], [ %45, %44 ]
  %10 = trunc nuw nsw i64 %9 to i32
  %11 = uitofp nneg i32 %10 to double
  %12 = fmul contract double %11, 1.000000e-01
  %13 = getelementptr inbounds nuw [10 x double], ptr %0, i64 %9
  %14 = getelementptr inbounds nuw [10 x double], ptr %1, i64 %9
  %15 = fmul contract double %11, 3.000000e-01
  %16 = getelementptr inbounds nuw [10 x double], ptr %2, i64 %9
  %17 = getelementptr inbounds nuw [10 x double], ptr %3, i64 %9
  %18 = fmul contract double %11, 5.000000e-01
  %19 = getelementptr inbounds nuw [10 x double], ptr %4, i64 %9
  %20 = getelementptr inbounds nuw [10 x double], ptr %5, i64 %9
  %21 = getelementptr inbounds nuw [10 x double], ptr %6, i64 %9
  br label %22

22:                                               ; preds = %8, %22
  %23 = phi i64 [ 0, %8 ], [ %42, %22 ]
  %24 = trunc nuw nsw i64 %23 to i32
  %25 = uitofp nneg i32 %24 to double
  %26 = fadd contract double %12, %25
  %27 = getelementptr inbounds nuw double, ptr %13, i64 %23
  store double %26, ptr %27, align 8, !tbaa !9
  %28 = fmul contract double %25, 2.000000e-01
  %29 = fadd contract double %28, %11
  %30 = getelementptr inbounds nuw double, ptr %14, i64 %23
  store double %29, ptr %30, align 8, !tbaa !9
  %31 = fadd contract double %15, %25
  %32 = getelementptr inbounds nuw double, ptr %16, i64 %23
  store double %31, ptr %32, align 8, !tbaa !9
  %33 = fmul contract double %25, 4.000000e-01
  %34 = fadd contract double %33, %11
  %35 = getelementptr inbounds nuw double, ptr %17, i64 %23
  store double %34, ptr %35, align 8, !tbaa !9
  %36 = fadd contract double %18, %25
  %37 = getelementptr inbounds nuw double, ptr %19, i64 %23
  store double %36, ptr %37, align 8, !tbaa !9
  %38 = fmul contract double %25, 6.000000e-01
  %39 = fadd contract double %38, %11
  %40 = getelementptr inbounds nuw double, ptr %20, i64 %23
  store double %39, ptr %40, align 8, !tbaa !9
  %41 = getelementptr inbounds nuw double, ptr %21, i64 %23
  store double 0.000000e+00, ptr %41, align 8, !tbaa !9
  %42 = add nuw nsw i64 %23, 1
  %43 = icmp eq i64 %42, 10
  br i1 %43, label %44, label %22, !llvm.loop !11

44:                                               ; preds = %22
  %45 = add nuw nsw i64 %9, 1
  %46 = icmp eq i64 %45, 10
  br i1 %46, label %47, label %8, !llvm.loop !14

47:                                               ; preds = %44
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
!10 = !{!"double", !7, i64 0}
!11 = distinct !{!11, !12, !13}
!12 = !{!"llvm.loop.mustprogress"}
!13 = !{!"llvm.loop.unroll.disable"}
!14 = distinct !{!14, !12, !13}
