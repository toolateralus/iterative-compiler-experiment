; ModuleID = 'program'
source_filename = "program"
target triple = "x86_64-pc-linux-gnu"

@str = private unnamed_addr constant [18 x i8] c"n = %d, n1 = %d\\n\00", align 1

define void @main() {
entry:
  %n1 = alloca i32, align 4
  store i32 10, ptr %n1, align 4
  %n = alloca i32, align 4
  %n11 = load i32, ptr %n1, align 4
  %multmp = mul i32 202, %n11
  store i32 %multmp, ptr %n, align 4
  %n2 = load i32, ptr %n, align 4
  %n13 = load i32, ptr %n1, align 4
  call void (ptr, ...) @printf(ptr @str, i32 %n2, i32 %n13)
  ret void
}

declare void @printf(ptr, ...)
