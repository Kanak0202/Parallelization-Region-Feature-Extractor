; ModuleID = 'extracted/capc_region_11.c'
source_filename = "extracted/capc_region_11.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-conda-linux-gnu"

; Function Attrs: nounwind uwtable
define dso_local void @capc_region_11(ptr noalias noundef writeonly captures(none) initializes((0, 8)) %0, ptr noalias noundef readonly captures(none) %1, ptr noalias noundef readonly captures(none) %2, ptr noalias noundef writeonly captures(none) initializes((0, 8)) %3, ptr noalias noundef readonly captures(none) %4, ptr noalias noundef writeonly captures(none) initializes((0, 8)) %5, ptr noalias noundef readonly captures(none) %6, ptr noalias noundef writeonly captures(none) initializes((0, 8)) %7, ptr noalias noundef readonly captures(none) %8, ptr noalias noundef writeonly captures(none) %9, ptr noalias noundef readonly captures(none) %10, ptr noalias noundef writeonly captures(none) %11, ptr noalias noundef readonly captures(none) %12, ptr noalias noundef writeonly captures(none) %13, ptr noalias noundef readonly captures(none) %14, ptr noalias noundef writeonly captures(none) %15, ptr noalias noundef readonly captures(none) %16, ptr noalias noundef writeonly captures(none) %17, double noundef %18, ptr noalias noundef writeonly captures(none) %19, ptr noalias noundef readonly captures(none) %20, ptr noalias noundef writeonly captures(none) %21, ptr noalias noundef writeonly captures(none) %22, ptr noalias noundef writeonly captures(none) %23, ptr noalias noundef %24, ptr noalias noundef writeonly captures(none) %25, ptr noalias noundef readonly captures(none) %26, ptr noalias noundef writeonly captures(none) %27, ptr noalias noundef writeonly captures(none) %28, ptr noalias noundef writeonly captures(none) %29, ptr noalias noundef writeonly captures(none) %30, ptr noalias noundef captures(none) %31, ptr noalias noundef writeonly captures(none) %32, ptr noalias noundef writeonly captures(none) %33, ptr noalias noundef writeonly captures(none) %34, ptr noalias noundef writeonly captures(none) %35, ptr noalias noundef captures(none) %36, ptr noalias noundef %37, ptr noalias noundef writeonly captures(none) %38, ptr noalias noundef readonly captures(none) %39, ptr noalias noundef readonly captures(none) %40, ptr noalias noundef writeonly captures(none) %41, ptr noalias noundef writeonly captures(none) %42, ptr noalias noundef writeonly captures(none) %43, ptr noalias noundef readonly captures(none) %44, ptr noalias noundef readonly captures(none) %45, ptr noalias noundef writeonly captures(none) %46, ptr noalias noundef readonly captures(none) %47, ptr noalias noundef writeonly captures(none) %48, ptr noalias noundef readonly captures(none) %49, ptr noalias noundef captures(none) %50) local_unnamed_addr #0 {
  %52 = getelementptr inbounds nuw i8, ptr %24, i64 8
  br label %53

53:                                               ; preds = %51, %178
  %54 = phi i64 [ -47442, %51 ], [ %222, %178 ]
  %55 = getelementptr inbounds double, ptr %1, i64 %54
  %56 = load double, ptr %55, align 8, !tbaa !9
  %57 = getelementptr inbounds double, ptr %2, i64 %54
  %58 = load double, ptr %57, align 8, !tbaa !9
  %59 = fadd contract double %56, %58
  store double %59, ptr %0, align 8, !tbaa !9
  %60 = getelementptr inbounds double, ptr %4, i64 %54
  %61 = load double, ptr %60, align 8, !tbaa !9
  %62 = fadd contract double %58, %61
  store double %62, ptr %3, align 8, !tbaa !9
  %63 = getelementptr inbounds double, ptr %6, i64 %54
  %64 = load double, ptr %63, align 8, !tbaa !9
  store double %64, ptr %5, align 8, !tbaa !9
  %65 = getelementptr inbounds double, ptr %8, i64 %54
  %66 = load double, ptr %65, align 8, !tbaa !9
  %67 = fmul contract double %66, %66
  %68 = fdiv contract double 1.000000e+00, %67
  store double %68, ptr %7, align 8, !tbaa !9
  %69 = getelementptr inbounds double, ptr %10, i64 %54
  %70 = getelementptr inbounds double, ptr %12, i64 %54
  %71 = getelementptr inbounds double, ptr %14, i64 %54
  %72 = getelementptr inbounds double, ptr %16, i64 %54
  %73 = getelementptr inbounds [5 x double], ptr %20, i64 %54
  %74 = getelementptr inbounds nuw i8, ptr %73, i64 8
  %75 = getelementptr inbounds nuw i8, ptr %73, i64 16
  %76 = getelementptr inbounds nuw i8, ptr %73, i64 24
  %77 = getelementptr inbounds nuw i8, ptr %73, i64 32
  %78 = getelementptr inbounds double, ptr %31, i64 %54
  %79 = getelementptr inbounds double, ptr %36, i64 %54
  %80 = trunc nsw i64 %54 to i32
  %81 = trunc nsw i64 %54 to i32
  %82 = trunc nsw i64 %54 to i32
  %83 = trunc nsw i64 %54 to i32
  br label %84

84:                                               ; preds = %53, %175
  %85 = phi i64 [ -47442, %53 ], [ %176, %175 ]
  %86 = icmp eq i64 %85, %54
  br i1 %86, label %175, label %87

87:                                               ; preds = %84
  %88 = load double, ptr %69, align 8, !tbaa !9
  %89 = getelementptr inbounds double, ptr %10, i64 %85
  %90 = load double, ptr %89, align 8, !tbaa !9
  %91 = fsub contract double %88, %90
  store double %91, ptr %9, align 8, !tbaa !9
  %92 = load double, ptr %70, align 8, !tbaa !9
  %93 = getelementptr inbounds double, ptr %12, i64 %85
  %94 = load double, ptr %93, align 8, !tbaa !9
  %95 = fsub contract double %92, %94
  store double %95, ptr %11, align 8, !tbaa !9
  %96 = load double, ptr %71, align 8, !tbaa !9
  %97 = getelementptr inbounds double, ptr %14, i64 %85
  %98 = load double, ptr %97, align 8, !tbaa !9
  %99 = fsub contract double %96, %98
  store double %99, ptr %13, align 8, !tbaa !9
  %100 = load double, ptr %72, align 8, !tbaa !9
  %101 = getelementptr inbounds double, ptr %16, i64 %85
  %102 = load double, ptr %101, align 8, !tbaa !9
  %103 = fsub contract double %100, %102
  store double %103, ptr %15, align 8, !tbaa !9
  %104 = fmul contract double %91, %91
  %105 = fmul contract double %95, %95
  %106 = fadd contract double %104, %105
  store double %106, ptr %17, align 8, !tbaa !9
  %107 = fcmp contract ugt double %106, %18
  br i1 %107, label %175, label %108

108:                                              ; preds = %87
  %109 = tail call contract double @sqrt(double noundef %106) #3
  store double %109, ptr %17, align 8, !tbaa !9
  %110 = load double, ptr %74, align 8, !tbaa !9
  store double %110, ptr %19, align 8, !tbaa !9
  %111 = load double, ptr %75, align 8, !tbaa !9
  store double %111, ptr %21, align 8, !tbaa !9
  %112 = load double, ptr %76, align 8, !tbaa !9
  store double %112, ptr %22, align 8, !tbaa !9
  %113 = load double, ptr %77, align 8, !tbaa !9
  store double %113, ptr %23, align 8, !tbaa !9
  tail call void @kernel(ptr noundef %24, double noundef %109) #3
  %114 = load double, ptr %24, align 8, !tbaa !9
  %115 = getelementptr inbounds double, ptr %26, i64 %85
  %116 = load double, ptr %115, align 8, !tbaa !9
  %117 = fmul contract double %114, %116
  %118 = getelementptr inbounds double, ptr %8, i64 %85
  %119 = load double, ptr %118, align 8, !tbaa !9
  %120 = fdiv contract double %117, %119
  %121 = load double, ptr %73, align 8, !tbaa !9
  %122 = fdiv contract double %120, %121
  store double %122, ptr %25, align 8, !tbaa !9
  %123 = fmul contract double %110, %113
  %124 = fmul contract double %111, %112
  %125 = fsub contract double %123, %124
  %126 = fdiv contract double 1.000000e+00, %125
  %127 = load double, ptr %52, align 8, !tbaa !9
  %128 = fmul contract double %113, %127
  %129 = fmul contract double %91, %128
  %130 = fdiv contract double %129, %109
  %131 = fmul contract double %111, %127
  %132 = fmul contract double %95, %131
  %133 = fdiv contract double %132, %109
  %134 = fsub contract double %130, %133
  %135 = fmul contract double %126, %134
  store double %135, ptr %27, align 8, !tbaa !9
  %136 = fmul contract double %110, %127
  %137 = fmul contract double %95, %136
  %138 = fdiv contract double %137, %109
  %139 = fmul contract double %112, %127
  %140 = fmul contract double %91, %139
  %141 = fdiv contract double %140, %109
  %142 = fsub contract double %138, %141
  %143 = fmul contract double %126, %142
  store double %143, ptr %28, align 8, !tbaa !9
  store double %119, ptr %29, align 8, !tbaa !9
  %144 = fmul contract double %109, %119
  store double %144, ptr %30, align 8, !tbaa !9
  %145 = fmul contract double %99, %135
  %146 = fmul contract double %103, %143
  %147 = fadd contract double %145, %146
  %148 = fmul contract double %116, %147
  %149 = load double, ptr %78, align 8, !tbaa !9
  %150 = fadd contract double %149, %148
  store double %150, ptr %78, align 8, !tbaa !9
  %151 = getelementptr inbounds double, ptr %1, i64 %85
  %152 = load double, ptr %151, align 8, !tbaa !9
  %153 = getelementptr inbounds double, ptr %2, i64 %85
  %154 = load double, ptr %153, align 8, !tbaa !9
  %155 = fadd contract double %152, %154
  store double %155, ptr %32, align 8, !tbaa !9
  %156 = getelementptr inbounds double, ptr %4, i64 %85
  %157 = load double, ptr %156, align 8, !tbaa !9
  %158 = fadd contract double %154, %157
  store double %158, ptr %33, align 8, !tbaa !9
  %159 = getelementptr inbounds double, ptr %6, i64 %85
  %160 = load double, ptr %159, align 8, !tbaa !9
  store double %160, ptr %34, align 8, !tbaa !9
  %161 = fmul contract double %119, %119
  %162 = fdiv contract double 1.000000e+00, %161
  store double %162, ptr %35, align 8, !tbaa !9
  %163 = trunc nsw i64 %85 to i32
  tail call void @monacorr(i32 noundef %80, i32 noundef %163, ptr noundef nonnull %24, double noundef %99, double noundef %103, double noundef %122, double noundef %135, double noundef %143) #3
  %164 = fmul contract double %116, 5.000000e-01
  %165 = fdiv contract double %164, %119
  %166 = fmul contract double %99, %143
  %167 = fmul contract double %103, %135
  %168 = fsub contract double %166, %167
  %169 = fmul contract double %165, %168
  %170 = load double, ptr %79, align 8, !tbaa !9
  %171 = fsub contract double %170, %169
  store double %171, ptr %79, align 8, !tbaa !9
  %172 = trunc nsw i64 %85 to i32
  tail call void @basicsph(i32 noundef %81, i32 noundef %172, ptr noundef nonnull %24, double noundef %91, double noundef %95, double noundef %99, double noundef %103, double noundef %68, double noundef %162, double noundef %109, double noundef %59, double noundef %62, double noundef %64, double noundef %155, double noundef %158, double noundef %160, double noundef %144, double noundef %122, double noundef %135, double noundef %143) #3
  %173 = trunc nsw i64 %85 to i32
  tail call void @viscosity(ptr noundef %37, i32 noundef %82, i32 noundef %173, double noundef %91, double noundef %95, double noundef %99, double noundef %103, double noundef %109, ptr noundef nonnull %24, double noundef %122, double noundef %135, double noundef %143) #3
  %174 = trunc nsw i64 %85 to i32
  tail call void @artificial_pressure(double noundef %58, double noundef %154, double noundef %68, double noundef %162, ptr noundef nonnull %24, double noundef %91, double noundef %95, double noundef %109, i32 noundef %83, i32 noundef %174, double noundef %99, double noundef %103, double noundef %122, double noundef %135, double noundef %143) #3
  br label %175

175:                                              ; preds = %84, %108, %87
  %176 = add nsw i64 %85, 1
  %177 = icmp eq i64 %176, 2558
  br i1 %177, label %178, label %84, !llvm.loop !11

178:                                              ; preds = %175
  %179 = getelementptr inbounds double, ptr %39, i64 %54
  %180 = load double, ptr %179, align 8, !tbaa !9
  %181 = getelementptr inbounds double, ptr %40, i64 %54
  %182 = load double, ptr %181, align 8, !tbaa !9
  %183 = fadd contract double %180, %182
  %184 = fmul contract double %183, 0xBFD5555555555555
  store double %184, ptr %38, align 8, !tbaa !9
  %185 = fadd contract double %180, %184
  %186 = fmul contract double %185, 1.385600e+01
  %187 = fmul contract double %64, 2.000000e+00
  %188 = getelementptr inbounds double, ptr %36, i64 %54
  %189 = load double, ptr %188, align 8, !tbaa !9
  %190 = fmul contract double %187, %189
  %191 = fadd contract double %190, %186
  %192 = getelementptr inbounds double, ptr %41, i64 %54
  store double %191, ptr %192, align 8, !tbaa !9
  %193 = fadd contract double %182, %184
  %194 = fmul contract double %193, 1.385600e+01
  %195 = fsub contract double %194, %190
  %196 = getelementptr inbounds double, ptr %42, i64 %54
  store double %195, ptr %196, align 8, !tbaa !9
  %197 = getelementptr inbounds double, ptr %44, i64 %54
  %198 = load double, ptr %197, align 8, !tbaa !9
  %199 = getelementptr inbounds double, ptr %45, i64 %54
  %200 = load double, ptr %199, align 8, !tbaa !9
  %201 = fadd contract double %198, %200
  %202 = fmul contract double %201, 6.928000e+00
  %203 = fsub contract double %56, %61
  %204 = fmul contract double %203, %189
  %205 = fsub contract double %202, %204
  %206 = getelementptr inbounds double, ptr %43, i64 %54
  store double %205, ptr %206, align 8, !tbaa !9
  %207 = getelementptr inbounds double, ptr %14, i64 %54
  %208 = load double, ptr %207, align 8, !tbaa !9
  %209 = getelementptr inbounds double, ptr %47, i64 %54
  %210 = load double, ptr %209, align 8, !tbaa !9
  %211 = fadd contract double %208, %210
  %212 = getelementptr inbounds double, ptr %46, i64 %54
  store double %211, ptr %212, align 8, !tbaa !9
  %213 = getelementptr inbounds double, ptr %16, i64 %54
  %214 = load double, ptr %213, align 8, !tbaa !9
  %215 = getelementptr inbounds double, ptr %49, i64 %54
  %216 = load double, ptr %215, align 8, !tbaa !9
  %217 = fadd contract double %214, %216
  %218 = getelementptr inbounds double, ptr %48, i64 %54
  store double %217, ptr %218, align 8, !tbaa !9
  %219 = getelementptr inbounds double, ptr %50, i64 %54
  %220 = load double, ptr %219, align 8, !tbaa !9
  %221 = fmul contract double %220, -5.000000e-01
  store double %221, ptr %219, align 8, !tbaa !9
  %222 = add nsw i64 %54, 1
  %223 = icmp eq i64 %222, 2558
  br i1 %223, label %224, label %53, !llvm.loop !14

224:                                              ; preds = %178
  ret void
}

; Function Attrs: nounwind
declare double @sqrt(double noundef) local_unnamed_addr #1

declare void @kernel(ptr noundef, double noundef) local_unnamed_addr #2

declare void @monacorr(i32 noundef, i32 noundef, ptr noundef, double noundef, double noundef, double noundef, double noundef, double noundef) local_unnamed_addr #2

declare void @basicsph(i32 noundef, i32 noundef, ptr noundef, double noundef, double noundef, double noundef, double noundef, double noundef, double noundef, double noundef, double noundef, double noundef, double noundef, double noundef, double noundef, double noundef, double noundef, double noundef, double noundef, double noundef) local_unnamed_addr #2

declare void @viscosity(ptr noundef, i32 noundef, i32 noundef, double noundef, double noundef, double noundef, double noundef, double noundef, ptr noundef, double noundef, double noundef, double noundef) local_unnamed_addr #2

declare void @artificial_pressure(double noundef, double noundef, double noundef, double noundef, ptr noundef, double noundef, double noundef, double noundef, i32 noundef, i32 noundef, double noundef, double noundef, double noundef, double noundef, double noundef) local_unnamed_addr #2

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
