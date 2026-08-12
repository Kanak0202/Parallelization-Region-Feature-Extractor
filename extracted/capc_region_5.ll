; ModuleID = 'extracted/capc_region_5.c'
source_filename = "extracted/capc_region_5.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-conda-linux-gnu"

; Function Attrs: nounwind uwtable
define dso_local void @capc_region_5(ptr noalias noundef writeonly captures(none) %0, ptr noalias noundef writeonly captures(none) %1, ptr noalias noundef writeonly captures(none) %2, ptr noalias noundef writeonly captures(none) %3, ptr noalias noundef writeonly captures(none) %4, ptr noalias noundef writeonly captures(none) %5, ptr noalias noundef writeonly captures(none) %6, ptr noalias noundef writeonly captures(none) %7, ptr noalias noundef writeonly captures(none) %8, ptr noalias noundef writeonly captures(none) %9, ptr noalias noundef writeonly captures(none) %10, ptr noalias noundef writeonly captures(none) %11, ptr noalias noundef writeonly captures(none) initializes((0, 8)) %12, ptr noalias noundef readonly captures(none) %13, ptr noalias noundef writeonly captures(none) %14, ptr noalias noundef writeonly captures(none) %15, ptr noalias noundef writeonly captures(none) initializes((0, 8)) %16) local_unnamed_addr #0 {
  br label %18

18:                                               ; preds = %17, %18
  %19 = phi i64 [ 0, %17 ], [ %42, %18 ]
  %20 = getelementptr inbounds nuw double, ptr %0, i64 %19
  store double 0.000000e+00, ptr %20, align 8, !tbaa !9
  %21 = getelementptr inbounds nuw double, ptr %2, i64 %19
  store double 0.000000e+00, ptr %21, align 8, !tbaa !9
  %22 = getelementptr inbounds nuw double, ptr %1, i64 %19
  store double 0.000000e+00, ptr %22, align 8, !tbaa !9
  %23 = getelementptr inbounds nuw double, ptr %6, i64 %19
  store double 0.000000e+00, ptr %23, align 8, !tbaa !9
  %24 = getelementptr inbounds nuw double, ptr %5, i64 %19
  store double 0.000000e+00, ptr %24, align 8, !tbaa !9
  %25 = getelementptr inbounds nuw double, ptr %4, i64 %19
  store double 0.000000e+00, ptr %25, align 8, !tbaa !9
  %26 = getelementptr inbounds nuw double, ptr %3, i64 %19
  store double 0.000000e+00, ptr %26, align 8, !tbaa !9
  %27 = getelementptr inbounds nuw double, ptr %7, i64 %19
  store double 0.000000e+00, ptr %27, align 8, !tbaa !9
  %28 = getelementptr inbounds nuw double, ptr %8, i64 %19
  store double 0.000000e+00, ptr %28, align 8, !tbaa !9
  %29 = getelementptr inbounds nuw double, ptr %10, i64 %19
  store double 0.000000e+00, ptr %29, align 8, !tbaa !9
  %30 = getelementptr inbounds nuw double, ptr %9, i64 %19
  store double 0.000000e+00, ptr %30, align 8, !tbaa !9
  %31 = getelementptr inbounds nuw double, ptr %11, i64 %19
  store double 0.000000e+00, ptr %31, align 8, !tbaa !9
  %32 = getelementptr inbounds nuw double, ptr %13, i64 %19
  %33 = load double, ptr %32, align 8, !tbaa !9
  %34 = fdiv contract double 1.154700e+00, %33
  store double %34, ptr %12, align 8, !tbaa !9
  %35 = fsub contract double 2.000000e+00, %34
  %36 = tail call contract double @pow(double noundef %35, double noundef 3.000000e+00) #2
  %37 = tail call contract double @pow(double noundef %35, double noundef 1.000000e+00) #2
  %38 = fsub contract double %36, %37
  %39 = fmul contract double %38, -1.385640e+01
  %40 = getelementptr inbounds nuw double, ptr %14, i64 %19
  store double %39, ptr %40, align 8, !tbaa !9
  %41 = getelementptr inbounds nuw double, ptr %15, i64 %19
  store double 0.000000e+00, ptr %41, align 8, !tbaa !9
  store double 0.000000e+00, ptr %16, align 8, !tbaa !9
  %42 = add nuw nsw i64 %19, 1
  %43 = icmp eq i64 %42, 2558
  br i1 %43, label %44, label %18, !llvm.loop !11

44:                                               ; preds = %18
  ret void
}

; Function Attrs: nounwind
declare double @pow(double noundef, double noundef) local_unnamed_addr #1

attributes #0 = { nounwind uwtable "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nounwind "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nobuiltin nounwind "no-builtins" }

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
