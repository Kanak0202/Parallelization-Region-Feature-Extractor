; ModuleID = 'extracted/capc_region_14.c'
source_filename = "extracted/capc_region_14.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-conda-linux-gnu"

; Function Attrs: nounwind uwtable
define dso_local void @capc_region_14(ptr noalias noundef writeonly captures(none) initializes((0, 8)) %0, ptr noalias noundef captures(none) %1, ptr noalias noundef readonly captures(none) %2, ptr noalias noundef writeonly captures(none) initializes((0, 8)) %3, ptr noalias noundef captures(none) %4, ptr noalias noundef writeonly captures(none) initializes((0, 8)) %5, ptr noalias noundef captures(none) %6, ptr noalias noundef writeonly captures(none) initializes((0, 8)) %7, ptr noalias noundef writeonly captures(none) %8, ptr noalias noundef writeonly captures(none) %9, ptr noalias noundef writeonly captures(none) %10, ptr noalias noundef writeonly captures(none) %11, ptr noalias noundef readonly captures(none) %12, ptr noalias noundef captures(none) %13, ptr noalias noundef writeonly captures(none) %14) local_unnamed_addr #0 {
  br label %16

16:                                               ; preds = %15, %72
  %17 = phi i64 [ 0, %15 ], [ %78, %72 ]
  %18 = getelementptr inbounds nuw double, ptr %1, i64 %17
  %19 = load double, ptr %18, align 8, !tbaa !9
  %20 = getelementptr inbounds nuw double, ptr %2, i64 %17
  %21 = load double, ptr %20, align 8, !tbaa !9
  %22 = fadd contract double %19, %21
  store double %22, ptr %0, align 8, !tbaa !9
  %23 = getelementptr inbounds nuw double, ptr %4, i64 %17
  %24 = load double, ptr %23, align 8, !tbaa !9
  %25 = fadd contract double %21, %24
  store double %25, ptr %3, align 8, !tbaa !9
  %26 = getelementptr inbounds nuw double, ptr %6, i64 %17
  %27 = load double, ptr %26, align 8, !tbaa !9
  store double %27, ptr %5, align 8, !tbaa !9
  %28 = fmul contract double %27, %27
  %29 = fsub contract double %22, %25
  %30 = fmul contract double %29, 2.500000e-01
  %31 = fmul contract double %29, %30
  %32 = fadd contract double %28, %31
  %33 = tail call contract double @sqrt(double noundef %32) #2
  store double %33, ptr %7, align 8, !tbaa !9
  %34 = fcmp contract ogt double %33, 1.360000e+00
  br i1 %34, label %35, label %62

35:                                               ; preds = %16
  %36 = fdiv contract double 1.360000e+00, %33
  store double %36, ptr %8, align 8, !tbaa !9
  %37 = fadd contract double %22, %25
  store double %37, ptr %9, align 8, !tbaa !9
  store double %29, ptr %10, align 8, !tbaa !9
  %38 = fmul contract double %27, %36
  store double %38, ptr %5, align 8, !tbaa !9
  %39 = fmul contract double %37, 5.000000e-01
  %40 = fmul contract double %36, 5.000000e-01
  %41 = fmul contract double %29, %40
  %42 = fadd contract double %39, %41
  store double %42, ptr %0, align 8, !tbaa !9
  %43 = fsub contract double %39, %41
  store double %43, ptr %3, align 8, !tbaa !9
  %44 = fsub contract double 1.000000e+00, %36
  %45 = fmul contract double %36, %44
  %46 = fmul contract double %27, 2.000000e+00
  %47 = fmul contract double %27, %46
  %48 = fmul contract double %19, %19
  %49 = fadd contract double %48, %47
  %50 = fmul contract double %24, %24
  %51 = fadd contract double %50, %49
  %52 = fmul contract double %51, %45
  %53 = getelementptr inbounds nuw double, ptr %12, i64 %17
  %54 = load double, ptr %53, align 8, !tbaa !9
  %55 = fmul contract double %52, %54
  %56 = fdiv contract double %55, 3.000000e+00
  %57 = fdiv contract double %56, 6.928000e+00
  %58 = getelementptr inbounds nuw double, ptr %13, i64 %17
  %59 = load double, ptr %58, align 8, !tbaa !9
  %60 = fdiv contract double %57, %59
  %61 = getelementptr inbounds nuw double, ptr %11, i64 %17
  store double %60, ptr %61, align 8, !tbaa !9
  br label %62

62:                                               ; preds = %35, %16
  %63 = phi double [ %38, %35 ], [ %27, %16 ]
  %64 = phi double [ %43, %35 ], [ %25, %16 ]
  %65 = phi double [ %42, %35 ], [ %22, %16 ]
  %66 = fadd contract double %65, %64
  %67 = fmul contract double %66, 5.000000e-01
  %68 = fcmp contract ogt double %67, 4.270000e+00
  br i1 %68, label %69, label %72

69:                                               ; preds = %62
  store double 0.000000e+00, ptr %5, align 8, !tbaa !9
  store double %21, ptr %0, align 8, !tbaa !9
  store double %21, ptr %3, align 8, !tbaa !9
  %70 = getelementptr inbounds nuw double, ptr %13, i64 %17
  store double 1.154700e+00, ptr %70, align 8, !tbaa !9
  %71 = getelementptr inbounds nuw double, ptr %14, i64 %17
  store double 0.000000e+00, ptr %71, align 8, !tbaa !9
  br label %72

72:                                               ; preds = %69, %62
  %73 = phi double [ 0.000000e+00, %69 ], [ %63, %62 ]
  %74 = phi double [ %21, %69 ], [ %64, %62 ]
  %75 = phi double [ %21, %69 ], [ %65, %62 ]
  %76 = fsub contract double %75, %21
  store double %76, ptr %18, align 8, !tbaa !9
  %77 = fsub contract double %74, %21
  store double %77, ptr %23, align 8, !tbaa !9
  store double %73, ptr %26, align 8, !tbaa !9
  %78 = add nuw nsw i64 %17, 1
  %79 = icmp eq i64 %78, 2558
  br i1 %79, label %80, label %16, !llvm.loop !11

80:                                               ; preds = %72
  ret void
}

; Function Attrs: nounwind
declare double @sqrt(double noundef) local_unnamed_addr #1

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
