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
void hello_world()
{
	Vector_3 dependent;
	dependent.z = 100;
	printf("dependent.z = '%d'\n", dependent.z);
	printf("Hello, World!\n");
}
int main()
{
	hello_world();
}
