; ModuleID = 'program'
source_filename = "program"
target triple = "x86_64-pc-linux-gnu"

@str = private unnamed_addr constant [39 x i8] c"n = %d, vector.x = %d, vector.y = %d\\n\00", align 1

declare void @printf(ptr, ...) local_unnamed_addr

define void @main() local_unnamed_addr {
entry:
  %vector.sroa.0 = alloca ptr, align 8
  store i32 100, ptr %vector.sroa.0, align 8
  %vector.sroa.0.4.sroa_idx11 = getelementptr inbounds i8, ptr %vector.sroa.0, i64 4
  store i32 200, ptr %vector.sroa.0.4.sroa_idx11, align 4
  %n = alloca i32, align 8
  store i32 225, ptr %n, align 8
  %n.0.n.0.n.0.n.0.n2 = load ptr, ptr %n, align 8
  %vector.sroa.0.0.vector.sroa.0.0.vector.sroa.0.0.vector.sroa.0.0.vector.sroa.0.0.load_dot_expr = load ptr, ptr %vector.sroa.0, align 8
  %vector.sroa.0.4.sroa_idx = getelementptr inbounds i8, ptr %vector.sroa.0, i64 4
  %vector.sroa.0.4.vector.sroa.0.4.vector.sroa.0.4.vector.sroa.0.4.vector.sroa.0.4.load_dot_expr5 = load ptr, ptr %vector.sroa.0.4.sroa_idx, align 4
  tail call void (ptr, ...) @printf(ptr nonnull @str, ptr %n.0.n.0.n.0.n.0.n2, ptr %vector.sroa.0.0.vector.sroa.0.0.vector.sroa.0.0.vector.sroa.0.0.vector.sroa.0.0.load_dot_expr, ptr %vector.sroa.0.4.vector.sroa.0.4.vector.sroa.0.4.vector.sroa.0.4.vector.sroa.0.4.load_dot_expr5)
  ret void
}
