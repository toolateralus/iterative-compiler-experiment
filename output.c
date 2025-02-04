typedef int i32;
typedef char * String;
typedef struct Vector_2 {
	i32 x;
	i32 y;
} Vector_2;
typedef struct Vector_3 {
	Vector_2 xy;
	i32 z;
} Vector_3;
extern void printf(String,... );
int main()
{
	Vector_3 dependent;
	dependent.z = 100;
	printf("%d\n", dependent.z);
}
