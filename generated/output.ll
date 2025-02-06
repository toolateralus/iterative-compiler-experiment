; ModuleID = 'program'
source_filename = "program"
target triple = "x86_64-pc-linux-gnu"

@str = private unnamed_addr constant [39 x i8] c"n = %d, vector.x = %d, vector.y = %d\\n\00", align 1

declare void @printf(ptr, ...) local_unnamed_addr

define void @main() local_unnamed_addr {
entry:
  %n = alloca i32, align 8
  store i32 225, ptr %n, align 8
  %n.0.n.0.n.0.n.0.n1 = load ptr, ptr %n, align 8
  tail call void (ptr, ...) @printf(ptr nonnull @str, ptr %n.0.n.0.n.0.n.0.n1, ptr null, ptr null)
  ret void
}
