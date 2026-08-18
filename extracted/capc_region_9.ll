; ModuleID = 'extracted/capc_region_9.c'
source_filename = "extracted/capc_region_9.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-conda-linux-gnu"

; Function Attrs: nounwind uwtable
define dso_local void @capc_region_9(ptr noalias noundef writeonly captures(none) %0, ptr noalias noundef readonly captures(none) %1, ptr noalias noundef writeonly captures(none) %2, ptr noalias noundef readonly captures(none) %3, ptr noalias noundef writeonly captures(none) %4, double noundef %5, ptr noalias noundef %6, ptr noalias noundef captures(none) %7, ptr noalias noundef readonly captures(none) %8, ptr noalias noundef readonly captures(none) %9) local_unnamed_addr #0 {
  %11 = getelementptr inbounds nuw i8, ptr %6, i64 8
  br label %12

12:                                               ; preds = %10, %80
  %13 = phi i64 [ -999997442, %10 ], [ %81, %80 ]
  %14 = getelementptr inbounds double, ptr %1, i64 %13
  %15 = getelementptr inbounds double, ptr %3, i64 %13
  %16 = getelementptr inbounds [5 x double], ptr %7, i64 %13
  %17 = getelementptr inbounds nuw i8, ptr %16, i64 8
  %18 = getelementptr inbounds nuw i8, ptr %16, i64 16
  %19 = getelementptr inbounds nuw i8, ptr %16, i64 24
  %20 = getelementptr inbounds nuw i8, ptr %16, i64 32
  br label %21

21:                                               ; preds = %12, %77
  %22 = phi i64 [ -999997442, %12 ], [ %78, %77 ]
  %23 = icmp eq i64 %22, %13
  br i1 %23, label %77, label %24

24:                                               ; preds = %21
  %25 = load double, ptr %14, align 8, !tbaa !9
  %26 = getelementptr inbounds double, ptr %1, i64 %22
  %27 = load double, ptr %26, align 8, !tbaa !9
  %28 = fsub contract double %25, %27
  store double %28, ptr %0, align 8, !tbaa !9
  %29 = load double, ptr %15, align 8, !tbaa !9
  %30 = getelementptr inbounds double, ptr %3, i64 %22
  %31 = load double, ptr %30, align 8, !tbaa !9
  %32 = fsub contract double %29, %31
  store double %32, ptr %2, align 8, !tbaa !9
  %33 = fmul contract double %28, %28
  %34 = fmul contract double %32, %32
  %35 = fadd contract double %33, %34
  store double %35, ptr %4, align 8, !tbaa !9
  %36 = fcmp contract ugt double %35, %5
  br i1 %36, label %77, label %37

37:                                               ; preds = %24
  %38 = tail call contract double @sqrt(double noundef %35) #3
  store double %38, ptr %4, align 8, !tbaa !9
  tail call void @kernel(ptr noundef %6, double noundef %38) #3
  %39 = load double, ptr %6, align 8, !tbaa !9
  %40 = getelementptr inbounds double, ptr %8, i64 %22
  %41 = load double, ptr %40, align 8, !tbaa !9
  %42 = fmul contract double %39, %41
  %43 = getelementptr inbounds double, ptr %9, i64 %22
  %44 = load double, ptr %43, align 8, !tbaa !9
  %45 = fdiv contract double %42, %44
  %46 = load double, ptr %16, align 8, !tbaa !9
  %47 = fadd contract double %46, %45
  store double %47, ptr %16, align 8, !tbaa !9
  %48 = fneg contract double %28
  %49 = load double, ptr %11, align 8, !tbaa !9
  %50 = fmul contract double %49, %48
  %51 = fmul contract double %28, %50
  %52 = fdiv contract double %51, %38
  %53 = fmul contract double %41, %52
  %54 = fdiv contract double %53, %44
  %55 = load double, ptr %17, align 8, !tbaa !9
  %56 = fadd contract double %55, %54
  store double %56, ptr %17, align 8, !tbaa !9
  %57 = fneg contract double %32
  %58 = fmul contract double %49, %57
  %59 = fmul contract double %28, %58
  %60 = fdiv contract double %59, %38
  %61 = fmul contract double %41, %60
  %62 = fdiv contract double %61, %44
  %63 = load double, ptr %18, align 8, !tbaa !9
  %64 = fadd contract double %63, %62
  store double %64, ptr %18, align 8, !tbaa !9
  %65 = fmul contract double %32, %50
  %66 = fdiv contract double %65, %38
  %67 = fmul contract double %41, %66
  %68 = fdiv contract double %67, %44
  %69 = load double, ptr %19, align 8, !tbaa !9
  %70 = fadd contract double %69, %68
  store double %70, ptr %19, align 8, !tbaa !9
  %71 = fmul contract double %32, %58
  %72 = fdiv contract double %71, %38
  %73 = fmul contract double %41, %72
  %74 = fdiv contract double %73, %44
  %75 = load double, ptr %20, align 8, !tbaa !9
  %76 = fadd contract double %74, %75
  store double %76, ptr %20, align 8, !tbaa !9
  br label %77

77:                                               ; preds = %21, %37, %24
  %78 = add nsw i64 %22, 1
  %79 = icmp eq i64 %78, 2558
  br i1 %79, label %80, label %21, !llvm.loop !11

80:                                               ; preds = %77
  %81 = add nsw i64 %13, 1
  %82 = icmp eq i64 %81, 2558
  br i1 %82, label %83, label %12, !llvm.loop !14

83:                                               ; preds = %80
  ret void
}

; Function Attrs: nounwind
declare double @sqrt(double noundef) local_unnamed_addr #1

declare void @kernel(ptr noundef, double noundef) local_unnamed_addr #2

attributes #0 = { nounwind uwtable "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nounwind "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { nobuiltin nounwind "no-builtins" }

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
