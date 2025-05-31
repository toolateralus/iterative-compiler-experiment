; ModuleID = 'program'
source_filename = "program"
target triple = "x86_64-pc-linux-gnu"

%Vector_3 = type { %Vector_2, i32 }
%Vector_2 = type { i32, i32 }

@str = private unnamed_addr constant [14 x i8] c"Hello, World\0A\00", align 1
@str.1 = private unnamed_addr constant [43 x i8] c".xy.x='%d', .xy.y='%d', .z='%d', sum='%d'\0A\00", align 1

define void @main() {
entry:
  call void @printf(ptr @str)
  %dependent = alloca %Vector_3, align 8
  %dependent1 = alloca %Vector_3, align 8
  %dotexpr = getelementptr inbounds %Vector_3, ptr %dependent1, i32 0, i32 0
  %dotexpr2 = getelementptr inbounds %Vector_2, ptr %dotexpr, i32 0, i32 1
  store i32 50, ptr %dotexpr2, align 4
  %dependent3 = alloca %Vector_3, align 8
  %dotexpr4 = getelementptr inbounds %Vector_3, ptr %dependent3, i32 0, i32 0
  %dotexpr5 = getelementptr inbounds %Vector_2, ptr %dotexpr4, i32 0, i32 0
  store i32 100, ptr %dotexpr5, align 4
  %dependent6 = alloca %Vector_3, align 8
  %dotexpr7 = getelementptr inbounds %Vector_3, ptr %dependent6, i32 0, i32 1
  store i32 200, ptr %dotexpr7, align 4
  %sum = alloca i32, align 4
  %dependent8 = alloca %Vector_3, align 8
  %dotexpr9 = getelementptr inbounds %Vector_3, ptr %dependent8, i32 0, i32 0
  %dotexpr10 = getelementptr inbounds %Vector_2, ptr %dotexpr9, i32 0, i32 1
  %load_dot_expr = load i32, ptr %dotexpr10, align 4
  %dependent11 = alloca %Vector_3, align 8
  %dotexpr12 = getelementptr inbounds %Vector_3, ptr %dependent11, i32 0, i32 0
  %dotexpr13 = getelementptr inbounds %Vector_2, ptr %dotexpr12, i32 0, i32 0
  %load_dot_expr14 = load i32, ptr %dotexpr13, align 4
  %addtmp = add i32 %load_dot_expr, %load_dot_expr14
  %dependent15 = alloca %Vector_3, align 8
  %dotexpr16 = getelementptr inbounds %Vector_3, ptr %dependent15, i32 0, i32 1
  %load_dot_expr17 = load i32, ptr %dotexpr16, align 4
  %addtmp18 = add i32 %addtmp, %load_dot_expr17
  store i32 %addtmp18, ptr %sum, align 4
  %dependent19 = alloca %Vector_3, align 8
  %dotexpr20 = getelementptr inbounds %Vector_3, ptr %dependent19, i32 0, i32 0
  %dotexpr21 = getelementptr inbounds %Vector_2, ptr %dotexpr20, i32 0, i32 0
  %load_dot_expr22 = load i32, ptr %dotexpr21, align 4
  %dependent23 = alloca %Vector_3, align 8
  %dotexpr24 = getelementptr inbounds %Vector_3, ptr %dependent23, i32 0, i32 0
  %dotexpr25 = getelementptr inbounds %Vector_2, ptr %dotexpr24, i32 0, i32 1
  %load_dot_expr26 = load i32, ptr %dotexpr25, align 4
  %dependent27 = alloca %Vector_3, align 8
  %dotexpr28 = getelementptr inbounds %Vector_3, ptr %dependent27, i32 0, i32 1
  %load_dot_expr29 = load i32, ptr %dotexpr28, align 4
  %sum30 = alloca i32, align 4
  %dependent31 = alloca %Vector_3, align 8
  %dotexpr32 = getelementptr inbounds %Vector_3, ptr %dependent31, i32 0, i32 0
  %dotexpr33 = getelementptr inbounds %Vector_2, ptr %dotexpr32, i32 0, i32 1
  %load_dot_expr34 = load i32, ptr %dotexpr33, align 4
  %dependent35 = alloca %Vector_3, align 8
  %dotexpr36 = getelementptr inbounds %Vector_3, ptr %dependent35, i32 0, i32 0
  %dotexpr37 = getelementptr inbounds %Vector_2, ptr %dotexpr36, i32 0, i32 0
  %load_dot_expr38 = load i32, ptr %dotexpr37, align 4
  %addtmp39 = add i32 %load_dot_expr34, %load_dot_expr38
  %dependent40 = alloca %Vector_3, align 8
  %dotexpr41 = getelementptr inbounds %Vector_3, ptr %dependent40, i32 0, i32 1
  %load_dot_expr42 = load i32, ptr %dotexpr41, align 4
  %addtmp43 = add i32 %addtmp39, %load_dot_expr42
  store i32 %addtmp43, ptr %sum30, align 4
  %sum44 = load i32, ptr %sum30, align 4
  call void @printf(ptr @str.1, i32 %load_dot_expr22, i32 %load_dot_expr26, i32 %load_dot_expr29, i32 %sum44)
  ret void
}

declare void @printf(ptr, ...)
