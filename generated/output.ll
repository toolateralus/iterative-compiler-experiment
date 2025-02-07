; ModuleID = 'program'
source_filename = "program"
target triple = "x86_64-pc-linux-gnu"

%Vector_2 = type { i32, i32 }

@str = private unnamed_addr constant [10 x i8] c"%d :: %d\0A\00", align 1

define void @main() {
entry:
  %output = alloca %Vector_2, align 8
  %dotexpr = getelementptr inbounds %Vector_2, ptr %output, i32 0, i32 0
  %load_dot_expr = load ptr, ptr %dotexpr, align 8
  store i32 0, ptr %load_dot_expr, align 4
  %dotexpr1 = getelementptr inbounds %Vector_2, ptr %output, i32 0, i32 1
  %load_dot_expr2 = load ptr, ptr %dotexpr1, align 8
  store i32 10, ptr %load_dot_expr2, align 4
  %dotexpr3 = getelementptr inbounds %Vector_2, ptr %output, i32 0, i32 0
  %load_dot_expr4 = load ptr, ptr %dotexpr3, align 8
  %dotexpr5 = getelementptr inbounds %Vector_2, ptr %output, i32 0, i32 1
  %load_dot_expr6 = load ptr, ptr %dotexpr5, align 8
  call void (ptr, ...) @printf(ptr @str, ptr %load_dot_expr4, ptr %load_dot_expr6)
  ret void
}

declare void @printf(ptr, ...)
