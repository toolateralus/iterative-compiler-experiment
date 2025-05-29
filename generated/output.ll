; ModuleID = 'program'
source_filename = "program"
target triple = "x86_64-pc-linux-gnu"

%Vector_3 = type { %Vector_2, i32 }
%Vector_2 = type { i32, i32 }

@str = private unnamed_addr constant [14 x i8] c"Hello, World\0A\00", align 1
@str.1 = private unnamed_addr constant [43 x i8] c".xy.x='%d', .xy.y='%d', .z='%d', sum='%d'\0A\00", align 1

define void @main() !dbg !4 {
entry:
  %0 = call i32 @clamp(), !dbg !7
  call void (ptr, ...) @printf(ptr @str), !dbg !8
  %dependent = alloca %Vector_3, align 8, !dbg !9
  %dotexpr = getelementptr inbounds %Vector_3, ptr %dependent, i32 0, i32 0, !dbg !10
  %dotexpr1 = getelementptr inbounds %Vector_2, ptr %dotexpr, i32 0, i32 1, !dbg !11
  store i32 50, ptr %dotexpr1, align 4
  %dotexpr2 = getelementptr inbounds %Vector_3, ptr %dependent, i32 0, i32 0, !dbg !12
  %dotexpr3 = getelementptr inbounds %Vector_2, ptr %dotexpr2, i32 0, i32 0, !dbg !13
  store i32 100, ptr %dotexpr3, align 4
  %dotexpr4 = getelementptr inbounds %Vector_3, ptr %dependent, i32 0, i32 1, !dbg !14
  store i32 200, ptr %dotexpr4, align 4
  %sum = alloca i32, align 4, !dbg !15
  %dotexpr5 = getelementptr inbounds %Vector_3, ptr %dependent, i32 0, i32 0, !dbg !16
  %dotexpr6 = getelementptr inbounds %Vector_2, ptr %dotexpr5, i32 0, i32 1, !dbg !17
  %load_dot_expr = load i32, ptr %dotexpr6, align 4, !dbg !17
  %dotexpr7 = getelementptr inbounds %Vector_3, ptr %dependent, i32 0, i32 0, !dbg !18
  %dotexpr8 = getelementptr inbounds %Vector_2, ptr %dotexpr7, i32 0, i32 0, !dbg !19
  %load_dot_expr9 = load i32, ptr %dotexpr8, align 4, !dbg !19
  %addtmp = add i32 %load_dot_expr, %load_dot_expr9, !dbg !20
  %dotexpr10 = getelementptr inbounds %Vector_3, ptr %dependent, i32 0, i32 1, !dbg !21
  %load_dot_expr11 = load i32, ptr %dotexpr10, align 4, !dbg !21
  %addtmp12 = add i32 %addtmp, %load_dot_expr11, !dbg !22
  store i32 %addtmp12, ptr %sum, align 4
  %dotexpr13 = getelementptr inbounds %Vector_3, ptr %dependent, i32 0, i32 0, !dbg !23
  %dotexpr14 = getelementptr inbounds %Vector_2, ptr %dotexpr13, i32 0, i32 0, !dbg !24
  %load_dot_expr15 = load i32, ptr %dotexpr14, align 4, !dbg !24
  %dotexpr16 = getelementptr inbounds %Vector_3, ptr %dependent, i32 0, i32 0, !dbg !25
  %dotexpr17 = getelementptr inbounds %Vector_2, ptr %dotexpr16, i32 0, i32 1, !dbg !26
  %load_dot_expr18 = load i32, ptr %dotexpr17, align 4, !dbg !26
  %dotexpr19 = getelementptr inbounds %Vector_3, ptr %dependent, i32 0, i32 1, !dbg !27
  %load_dot_expr20 = load i32, ptr %dotexpr19, align 4, !dbg !27
  %sum21 = load i32, ptr %sum, align 4
  call void (ptr, ...) @printf(ptr @str.1, i32 %load_dot_expr15, i32 %load_dot_expr18, i32 %load_dot_expr20, i32 %sum21), !dbg !28
  ret void
}

define i32 @clamp() !dbg !29 {
entry:
  ret i32 0, !dbg !30
}

declare void @printf(ptr, ...)

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "Iterative", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false)
!1 = !DIFile(filename: "max.it", directory: "/home/josh_arch/source/c/iterative-compiler/")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = !{i32 2, !"Dwarf Versio", i32 4}
!4 = distinct !DISubprogram(name: "main", linkageName: "main", scope: !1, file: !1, line: 2, type: !5, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !0)
!5 = !DISubroutineType(types: !6)
!6 = !{}
!7 = !DILocation(line: 3, column: 29, scope: !4)
!8 = !DILocation(line: 5, column: 13, scope: !4)
!9 = !DILocation(line: 5, column: 22, scope: !4)
!10 = !DILocation(line: 6, column: 13, scope: !4)
!11 = !DILocation(line: 6, column: 17, scope: !4)
!12 = !DILocation(line: 7, column: 13, scope: !4)
!13 = !DILocation(line: 7, column: 16, scope: !4)
!14 = !DILocation(line: 8, column: 14, scope: !4)
!15 = !DILocation(line: 8, column: 34, scope: !4)
!16 = !DILocation(line: 8, column: 49, scope: !4)
!17 = !DILocation(line: 8, column: 52, scope: !4)
!18 = !DILocation(line: 8, column: 68, scope: !4)
!19 = !DILocation(line: 9, column: 10, scope: !4)
!20 = !DILocation(line: 9, column: 56, scope: !4)
!21 = !DILocation(line: 9, column: 68, scope: !4)
!22 = !DILocation(line: 9, column: 72, scope: !4)
!23 = !DILocation(line: 9, column: 91, scope: !4)
!24 = !DILocation(line: 9, column: 94, scope: !4)
!25 = !DILocation(line: 9, column: 109, scope: !4)
!26 = !DILocation(line: 9, column: 115, scope: !4)
!27 = !DILocation(line: 12, column: 4, scope: !4)
!28 = !DILocation(line: 9, column: 75, scope: !4)
!29 = distinct !DISubprogram(name: "clamp", linkageName: "clamp", scope: !1, file: !1, line: 13, type: !5, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !0)
!30 = !DILocation(line: 16, column: 26, scope: !29)
