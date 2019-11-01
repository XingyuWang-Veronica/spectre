#include <stdio.h>
#include "utils.h"

/*int add(int x, int y) {
	return x + y;
}*/

int main() {
	int x = 1, y = 2;
	int (*func_ptr)(int, int) = &add;
	int result = (*add)(x, y);
	printf("result is %d\n", result);
	return 0;
}
