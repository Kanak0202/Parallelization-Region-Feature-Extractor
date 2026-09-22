; ModuleID = 'extracted/capc_region_2.c'
source_filename = "extracted/capc_region_2.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-conda-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable
define dso_local void @capc_region_2(ptr noalias noundef writeonly captures(none) %0, ptr noalias noundef readonly captures(none) %1) local_unnamed_addr #0 {
  br label %3

3:                                                ; preds = %2, %60
  %4 = phi i64 [ 1, %2 ], [ %61, %60 ]
  %5 = getelementptr inbounds nuw [17 x [17 x float]], ptr %1, i64 %4
  %6 = getelementptr inbounds nuw i8, ptr %5, i64 1156
  %7 = getelementptr i8, ptr %5, i64 -1156
  %8 = getelementptr inbounds nuw [17 x [17 x float]], ptr %0, i64 %4
  br label %9

9:                                                ; preds = %3, %57
  %10 = phi i64 [ 1, %3 ], [ %58, %57 ]
  %11 = getelementptr inbounds nuw [17 x float], ptr %6, i64 %10
  %12 = getelementptr inbounds nuw [17 x float], ptr %5, i64 %10
  %13 = getelementptr inbounds nuw [17 x float], ptr %7, i64 %10
  %14 = getelementptr inbounds nuw i8, ptr %12, i64 68
  %15 = getelementptr i8, ptr %12, i64 -68
  %16 = getelementptr inbounds nuw [17 x float], ptr %8, i64 %10
  br label %17

17:                                               ; preds = %9, %17
  %18 = phi i64 [ 1, %9 ], [ %42, %17 ]
  %19 = getelementptr inbounds nuw float, ptr %11, i64 %18
  %20 = load float, ptr %19, align 4, !tbaa !9
  %21 = fpext contract float %20 to double
  %22 = getelementptr inbounds nuw float, ptr %12, i64 %18
  %23 = load float, ptr %22, align 4, !tbaa !9
  %24 = fpext contract float %23 to double
  %25 = fmul contract double %24, 2.000000e+00
  %26 = fsub contract double %21, %25
  %27 = getelementptr inbounds nuw float, ptr %13, i64 %18
  %28 = load float, ptr %27, align 4, !tbaa !9
  %29 = fpext contract float %28 to double
  %30 = fadd contract double %26, %29
  %31 = fmul contract double %30, 1.250000e-01
  %32 = getelementptr inbounds nuw float, ptr %14, i64 %18
  %33 = load float, ptr %32, align 4, !tbaa !9
  %34 = fpext contract float %33 to double
  %35 = fsub contract double %34, %25
  %36 = getelementptr inbounds nuw float, ptr %15, i64 %18
  %37 = load float, ptr %36, align 4, !tbaa !9
  %38 = fpext contract float %37 to double
  %39 = fadd contract double %35, %38
  %40 = fmul contract double %39, 1.250000e-01
  %41 = fadd contract double %31, %40
  %42 = add nuw nsw i64 %18, 1
  %43 = getelementptr inbounds nuw float, ptr %12, i64 %42
  %44 = load float, ptr %43, align 4, !tbaa !9
  %45 = fpext contract float %44 to double
  %46 = fsub contract double %45, %25
  %47 = getelementptr i8, ptr %22, i64 -4
  %48 = load float, ptr %47, align 4, !tbaa !9
  %49 = fpext contract float %48 to double
  %50 = fadd contract double %46, %49
  %51 = fmul contract double %50, 1.250000e-01
  %52 = fadd contract double %41, %51
  %53 = fadd contract double %52, %24
  %54 = fptrunc contract double %53 to float
  %55 = getelementptr inbounds nuw float, ptr %16, i64 %18
  store float %54, ptr %55, align 4, !tbaa !9
  %56 = icmp eq i64 %42, 16
  br i1 %56, label %57, label %17, !llvm.loop !11

57:                                               ; preds = %17
  %58 = add nuw nsw i64 %10, 1
  %59 = icmp eq i64 %58, 16
  br i1 %59, label %60, label %9, !llvm.loop !14

60:                                               ; preds = %57
  %61 = add nuw nsw i64 %4, 1
  %62 = icmp eq i64 %61, 16
  br i1 %62, label %63, label %3, !llvm.loop !15

63:                                               ; preds = %60
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
!10 = !{!"float", !7, i64 0}
!11 = distinct !{!11, !12, !13}
!12 = !{!"llvm.loop.mustprogress"}
!13 = !{!"llvm.loop.unroll.disable"}
!14 = distinct !{!14, !12, !13}
!15 = distinct !{!15, !12, !13}
