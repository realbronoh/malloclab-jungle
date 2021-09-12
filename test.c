#include <stdio.h>

#define ALIGNMENT 8
#define __SIZE_TYPE__ long unsigned int
typedef __SIZE_TYPE__ size_t;
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

int main(void){
    int newsize = ALIGN(9 + SIZE_T_SIZE);
    // printf("%ld\n", SIZE_T_SIZE);
    // printf("%d\n", (int)sizeof(long unsigned int));
    printf("%d\n", newsize);

    return 0;
}