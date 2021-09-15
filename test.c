#include <stdio.h>
#include <stdlib.h>


int main(void){
    int n = 1000;
    void *ptr = malloc(4 * n);
    *((int *)ptr + 997) = 55555;
    for (int i=0; i<n; i++){
        printf("%d-idx is %d\n", i, *((int *)ptr + i));
    }

    return 0;
}