; ModuleID = 'program'
source_filename = "program"
target triple = "x86_64-pc-linux-gnu"

%Vector_2 = type { i32, i32 }
%Vector_3 = type { %Vector_3, i32 }

@str = private unnamed_addr constant [12 x i8] c"x=%d, y=%d\0A\00", align 1

define void @main() {
entry:
  %output = alloca %Vector_2, align 8
  %0 = call %Vector_2 @get_Vector_2()
  store %Vector_2 %0, ptr %output, align 4
  %dotexpr = getelementptr inbounds %Vector_2, ptr %output, i32 0, i32 0
  store i32 0, ptr %dotexpr, align 4
  %dotexpr1 = getelementptr inbounds %Vector_2, ptr %output, i32 0, i32 1
  store i32 10, ptr %dotexpr1, align 4
  %dotexpr2 = getelementptr inbounds %Vector_2, ptr %output, i32 0, i32 0
  %dotexpr3 = getelementptr inbounds %Vector_2, ptr %output, i32 0, i32 0
  %load_dot_expr = load i32, ptr %dotexpr3, align 4
  %dotexpr4 = getelementptr inbounds %Vector_2, ptr %output, i32 0, i32 0
  %load_dot_expr5 = load i32, ptr %dotexpr4, align 4
  %addtmp = add i32 %load_dot_expr, %load_dot_expr5
  store i32 %addtmp, ptr %dotexpr2, align 4
  %dotexpr6 = getelementptr inbounds %Vector_2, ptr %output, i32 0, i32 0
  %load_dot_expr7 = load i32, ptr %dotexpr6, align 4
  %dotexpr8 = getelementptr inbounds %Vector_2, ptr %output, i32 0, i32 1
  %load_dot_expr9 = load i32, ptr %dotexpr8, align 4
  call void (ptr, ...) @printf(ptr @str, i32 %load_dot_expr7, i32 %load_dot_expr9)
  %vector = alloca %Vector_3, align 8
  ret void
}

define %Vector_2 @get_Vector_2() {
entry:
  %vector = alloca %Vector_2, align 8
  %vector1 = load %Vector_2, ptr %vector, align 4
  ret %Vector_2 %vector1
}

declare void @printf(ptr, ...)
