; ModuleID = 'extracted/capc_region_10.c'
source_filename = "extracted/capc_region_10.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-conda-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable
define dso_local void @capc_region_10(ptr noalias noundef writeonly captures(none) %0, ptr noalias noundef readonly captures(none) %1, ptr noalias noundef readonly captures(none) %2, ptr noalias noundef writeonly captures(none) %3, ptr noalias noundef readonly captures(none) %4, ptr noalias noundef writeonly captures(none) %5, ptr noalias noundef readonly captures(none) %6, ptr noalias noundef writeonly captures(none) %7, ptr noalias noundef readonly captures(none) %8, ptr noalias noundef readnone captures(none) %9, ptr noalias noundef readnone captures(none) %10, ptr noalias noundef readnone captures(none) %11, ptr noalias noundef readnone captures(none) %12, ptr noalias noundef readnone captures(none) %13, ptr noalias noundef readnone captures(none) %14, ptr noalias noundef readnone captures(none) %15, ptr noalias noundef readnone captures(none) %16, ptr noalias noundef readnone captures(none) %17, ptr noalias noundef readnone captures(none) %18, ptr noalias noundef readnone captures(none) %19, ptr noalias noundef readnone captures(none) %20, ptr noalias noundef readnone captures(none) %21, ptr noalias noundef readonly captures(none) %22, ptr noalias noundef readnone captures(none) %23, ptr noalias noundef readonly captures(none) %24, double noundef %25, ptr noalias noundef readnone captures(none) %26, ptr noalias noundef readnone captures(none) %27, ptr noalias noundef readnone captures(none) %28, ptr noalias noundef readnone captures(none) %29, ptr noalias noundef readnone captures(none) %30, ptr noalias noundef readnone captures(none) %31, ptr noalias noundef readnone captures(none) %32, ptr noalias noundef readnone captures(none) %33, ptr noalias noundef readnone captures(none) %34, ptr noalias noundef readnone captures(none) %35, ptr noalias noundef readnone captures(none) %36, ptr noalias noundef readnone captures(none) %37, ptr noalias noundef readnone captures(none) %38, ptr noalias noundef readnone captures(none) %39, ptr noalias noundef readnone captures(none) %40, ptr noalias noundef readnone captures(none) %41, ptr noalias noundef readonly captures(none) %42, ptr noalias noundef readnone captures(none) %43, ptr noalias noundef writeonly captures(none) %44, ptr noalias noundef readonly captures(none) %45, ptr noalias noundef readonly captures(none) %46, ptr noalias noundef writeonly captures(none) %47, ptr noalias noundef writeonly captures(none) %48, ptr noalias noundef writeonly captures(none) %49, ptr noalias noundef readonly captures(none) %50, ptr noalias noundef readonly captures(none) %51, ptr noalias noundef writeonly captures(none) %52, ptr noalias noundef readonly captures(none) %53, ptr noalias noundef writeonly captures(none) %54, ptr noalias noundef readonly captures(none) %55, ptr noalias noundef captures(none) %56) local_unnamed_addr #0 {
  br label %58

58:                                               ; preds = %57, %58
  %59 = phi i64 [ 0, %57 ], [ %109, %58 ]
  %60 = getelementptr inbounds nuw double, ptr %1, i64 %59
  %61 = load double, ptr %60, align 8, !tbaa !9
  %62 = getelementptr inbounds nuw double, ptr %4, i64 %59
  %63 = load double, ptr %62, align 8, !tbaa !9
  %64 = getelementptr inbounds nuw double, ptr %6, i64 %59
  %65 = load double, ptr %64, align 8, !tbaa !9
  %66 = getelementptr inbounds nuw double, ptr %45, i64 %59
  %67 = load double, ptr %66, align 8, !tbaa !9
  %68 = getelementptr inbounds nuw double, ptr %46, i64 %59
  %69 = load double, ptr %68, align 8, !tbaa !9
  %70 = fadd contract double %67, %69
  %71 = fmul contract double %70, 0xBFD5555555555555
  %72 = fadd contract double %67, %71
  %73 = fmul contract double %72, 1.385600e+01
  %74 = fmul contract double %65, 2.000000e+00
  %75 = getelementptr inbounds nuw double, ptr %42, i64 %59
  %76 = load double, ptr %75, align 8, !tbaa !9
  %77 = fmul contract double %74, %76
  %78 = fadd contract double %77, %73
  %79 = getelementptr inbounds nuw double, ptr %47, i64 %59
  store double %78, ptr %79, align 8, !tbaa !9
  %80 = fadd contract double %69, %71
  %81 = fmul contract double %80, 1.385600e+01
  %82 = fsub contract double %81, %77
  %83 = getelementptr inbounds nuw double, ptr %48, i64 %59
  store double %82, ptr %83, align 8, !tbaa !9
  %84 = getelementptr inbounds nuw double, ptr %50, i64 %59
  %85 = load double, ptr %84, align 8, !tbaa !9
  %86 = getelementptr inbounds nuw double, ptr %51, i64 %59
  %87 = load double, ptr %86, align 8, !tbaa !9
  %88 = fadd contract double %85, %87
  %89 = fmul contract double %88, 6.928000e+00
  %90 = fsub contract double %61, %63
  %91 = fmul contract double %90, %76
  %92 = fsub contract double %89, %91
  %93 = getelementptr inbounds nuw double, ptr %49, i64 %59
  store double %92, ptr %93, align 8, !tbaa !9
  %94 = getelementptr inbounds nuw double, ptr %22, i64 %59
  %95 = load double, ptr %94, align 8, !tbaa !9
  %96 = getelementptr inbounds nuw double, ptr %53, i64 %59
  %97 = load double, ptr %96, align 8, !tbaa !9
  %98 = fadd contract double %95, %97
  %99 = getelementptr inbounds nuw double, ptr %52, i64 %59
  store double %98, ptr %99, align 8, !tbaa !9
  %100 = getelementptr inbounds nuw double, ptr %24, i64 %59
  %101 = load double, ptr %100, align 8, !tbaa !9
  %102 = getelementptr inbounds nuw double, ptr %55, i64 %59
  %103 = load double, ptr %102, align 8, !tbaa !9
  %104 = fadd contract double %101, %103
  %105 = getelementptr inbounds nuw double, ptr %54, i64 %59
  store double %104, ptr %105, align 8, !tbaa !9
  %106 = getelementptr inbounds nuw double, ptr %56, i64 %59
  %107 = load double, ptr %106, align 8, !tbaa !9
  %108 = fmul contract double %107, -5.000000e-01
  store double %108, ptr %106, align 8, !tbaa !9
  %109 = add nuw nsw i64 %59, 1
  %110 = icmp eq i64 %109, 2558
  br i1 %110, label %111, label %58, !llvm.loop !11

111:                                              ; preds = %58
  %112 = getelementptr inbounds nuw i8, ptr %2, i64 20456
  %113 = load double, ptr %112, align 8, !tbaa !9
  %114 = fadd contract double %61, %113
  %115 = fadd contract double %113, %63
  %116 = getelementptr inbounds nuw i8, ptr %8, i64 20456
  %117 = load double, ptr %116, align 8, !tbaa !9
  %118 = fmul contract double %117, %117
  %119 = fdiv contract double 1.000000e+00, %118
  store double %114, ptr %0, align 8, !tbaa !9
  store double %115, ptr %3, align 8, !tbaa !9
  store double %65, ptr %5, align 8, !tbaa !9
  store double %119, ptr %7, align 8, !tbaa !9
  store double %71, ptr %44, align 8, !tbaa !9
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
