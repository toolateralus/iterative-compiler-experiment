; ModuleID = 'program'
source_filename = "program"
target triple = "x86_64-pc-linux-gnu"

%Vector_2 = type { i32, i32 }

@str = private unnamed_addr constant [39 x i8] c"n = %d, vector.x = %d, vector.y = %d\\n\00", align 1

declare void @printf(ptr, ...)

define void @main() {
entry:
  %vector = alloca %Vector_2, align 8
  %dot_expr = getelementptr inbounds %Vector_2, ptr %vector, i32 0, i32 0
  store i32 100, ptr %dot_expr, align 4
  %dot_expr1 = getelementptr inbounds %Vector_2, ptr %vector, i32 0, i32 1
  store i32 200, ptr %dot_expr1, align 4
  %n = alloca i32, align 4
  store i32 225, ptr %n, align 4
  %n2 = load ptr, ptr %n, align 8
  %dot_expr3 = getelementptr inbounds %Vector_2, ptr %vector, i32 0, i32 0
  %load_dot_expr = load ptr, ptr %dot_expr3, align 8
  %dot_expr4 = getelementptr inbounds %Vector_2, ptr %vector, i32 0, i32 1
  %load_dot_expr5 = load ptr, ptr %dot_expr4, align 8
  call void (ptr, ...) @printf(ptr @str, ptr %n2, ptr %load_dot_expr, ptr %load_dot_expr5)
  ret void
}
