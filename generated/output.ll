; ModuleID = 'program'
source_filename = "program"
target triple = "x86_64-pc-linux-gnu"

%Vector_2 = type { i32, i32 }

@str = private unnamed_addr constant [16 x i8] c"Xaryu Was Here\0A\00", align 1
@str.1 = private unnamed_addr constant [10 x i8] c"%d :: %d\0A\00", align 1
@str.2 = private unnamed_addr constant [10 x i8] c"end main\0A\00", align 1

define void @main() {
entry:
  %output = alloca %Vector_2, align 8
  call void (ptr, ...) @printf(ptr @str)
  %dotexpr = getelementptr inbounds %Vector_2, ptr %output, i32 0, i32 0
  store i32 0, ptr %dotexpr, align 4
  %dotexpr1 = getelementptr inbounds %Vector_2, ptr %output, i32 0, i32 1
  store i32 10, ptr %dotexpr1, align 4
  %dotexpr2 = getelementptr inbounds %Vector_2, ptr %output, i32 0, i32 0
  %load_dot_expr = load i32, ptr %dotexpr2, align 4
  %dotexpr3 = getelementptr inbounds %Vector_2, ptr %output, i32 0, i32 1
  %load_dot_expr4 = load i32, ptr %dotexpr3, align 4
  call void (ptr, ...) @printf(ptr @str.1, i32 %load_dot_expr, i32 %load_dot_expr4)
  call void (ptr, ...) @printf(ptr @str.2)
  ret void
}

declare void @printf(ptr, ...)
