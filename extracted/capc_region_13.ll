; ModuleID = 'extracted/capc_region_13.c'
source_filename = "extracted/capc_region_13.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-conda-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable
define dso_local void @capc_region_13(ptr noalias noundef writeonly captures(none) %0, ptr noalias noundef readonly captures(none) %1, double noundef %2, ptr noalias noundef readonly captures(none) %3, ptr noalias noundef writeonly captures(none) %4, ptr noalias noundef readonly captures(none) %5, ptr noalias noundef readonly captures(none) %6, ptr noalias noundef writeonly captures(none) %7, ptr noalias noundef readonly captures(none) %8, ptr noalias noundef readonly captures(none) %9, ptr noalias noundef writeonly captures(none) %10, ptr noalias noundef readonly captures(none) %11, ptr noalias noundef readonly captures(none) %12, ptr noalias noundef writeonly captures(none) %13, ptr noalias noundef readonly captures(none) %14, ptr noalias noundef readonly captures(none) %15, ptr noalias noundef writeonly captures(none) %16, ptr noalias noundef readonly captures(none) %17, ptr noalias noundef readonly captures(none) %18, ptr noalias noundef writeonly captures(none) %19, ptr noalias noundef readonly captures(none) %20, ptr noalias noundef readonly captures(none) %21, ptr noalias noundef writeonly captures(none) %22, ptr noalias noundef readonly captures(none) %23, ptr noalias noundef readonly captures(none) %24, ptr noalias noundef writeonly captures(none) %25, ptr noalias noundef readonly captures(none) %26, ptr noalias noundef readonly captures(none) %27) local_unnamed_addr #0 {
  %29 = fmul contract double %2, 5.000000e-01
  br label %30

30:                                               ; preds = %28, %30
  %31 = phi i64 [ 0, %28 ], [ %95, %30 ]
  %32 = getelementptr inbounds nuw double, ptr %1, i64 %31
  %33 = load double, ptr %32, align 8, !tbaa !9
  %34 = getelementptr inbounds nuw double, ptr %3, i64 %31
  %35 = load double, ptr %34, align 8, !tbaa !9
  %36 = fmul contract double %29, %35
  %37 = fadd contract double %33, %36
  %38 = getelementptr inbounds nuw double, ptr %0, i64 %31
  store double %37, ptr %38, align 8, !tbaa !9
  %39 = getelementptr inbounds nuw double, ptr %5, i64 %31
  %40 = load double, ptr %39, align 8, !tbaa !9
  %41 = getelementptr inbounds nuw double, ptr %6, i64 %31
  %42 = load double, ptr %41, align 8, !tbaa !9
  %43 = fmul contract double %29, %42
  %44 = fadd contract double %40, %43
  %45 = getelementptr inbounds nuw double, ptr %4, i64 %31
  store double %44, ptr %45, align 8, !tbaa !9
  %46 = getelementptr inbounds nuw double, ptr %8, i64 %31
  %47 = load double, ptr %46, align 8, !tbaa !9
  %48 = getelementptr inbounds nuw double, ptr %9, i64 %31
  %49 = load double, ptr %48, align 8, !tbaa !9
  %50 = fmul contract double %29, %49
  %51 = fadd contract double %47, %50
  %52 = getelementptr inbounds nuw double, ptr %7, i64 %31
  store double %51, ptr %52, align 8, !tbaa !9
  %53 = getelementptr inbounds nuw double, ptr %11, i64 %31
  %54 = load double, ptr %53, align 8, !tbaa !9
  %55 = getelementptr inbounds nuw double, ptr %12, i64 %31
  %56 = load double, ptr %55, align 8, !tbaa !9
  %57 = fmul contract double %29, %56
  %58 = fadd contract double %54, %57
  %59 = getelementptr inbounds nuw double, ptr %10, i64 %31
  store double %58, ptr %59, align 8, !tbaa !9
  %60 = getelementptr inbounds nuw double, ptr %14, i64 %31
  %61 = load double, ptr %60, align 8, !tbaa !9
  %62 = getelementptr inbounds nuw double, ptr %15, i64 %31
  %63 = load double, ptr %62, align 8, !tbaa !9
  %64 = fmul contract double %29, %63
  %65 = fadd contract double %61, %64
  %66 = getelementptr inbounds nuw double, ptr %13, i64 %31
  store double %65, ptr %66, align 8, !tbaa !9
  %67 = getelementptr inbounds nuw double, ptr %17, i64 %31
  %68 = load double, ptr %67, align 8, !tbaa !9
  %69 = getelementptr inbounds nuw double, ptr %18, i64 %31
  %70 = load double, ptr %69, align 8, !tbaa !9
  %71 = fmul contract double %29, %70
  %72 = fadd contract double %68, %71
  %73 = getelementptr inbounds nuw double, ptr %16, i64 %31
  store double %72, ptr %73, align 8, !tbaa !9
  %74 = getelementptr inbounds nuw double, ptr %20, i64 %31
  %75 = load double, ptr %74, align 8, !tbaa !9
  %76 = getelementptr inbounds nuw double, ptr %21, i64 %31
  %77 = load double, ptr %76, align 8, !tbaa !9
  %78 = fmul contract double %29, %77
  %79 = fadd contract double %75, %78
  %80 = getelementptr inbounds nuw double, ptr %19, i64 %31
  store double %79, ptr %80, align 8, !tbaa !9
  %81 = getelementptr inbounds nuw double, ptr %23, i64 %31
  %82 = load double, ptr %81, align 8, !tbaa !9
  %83 = getelementptr inbounds nuw double, ptr %24, i64 %31
  %84 = load double, ptr %83, align 8, !tbaa !9
  %85 = fmul contract double %29, %84
  %86 = fadd contract double %82, %85
  %87 = getelementptr inbounds nuw double, ptr %22, i64 %31
  store double %86, ptr %87, align 8, !tbaa !9
  %88 = getelementptr inbounds nuw double, ptr %26, i64 %31
  %89 = load double, ptr %88, align 8, !tbaa !9
  %90 = getelementptr inbounds nuw double, ptr %27, i64 %31
  %91 = load double, ptr %90, align 8, !tbaa !9
  %92 = fmul contract double %29, %91
  %93 = fadd contract double %89, %92
  %94 = getelementptr inbounds nuw double, ptr %25, i64 %31
  store double %93, ptr %94, align 8, !tbaa !9
  %95 = add nuw nsw i64 %31, 1
  %96 = icmp eq i64 %95, 2558
  br i1 %96, label %97, label %30, !llvm.loop !11

97:                                               ; preds = %30
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
