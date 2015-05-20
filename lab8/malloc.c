#include <stdio.h>
#include <stdlib.h>
int main(void)
{
	int *p = malloc(sizeof(int)*50000000);
	int i, j;
	for(i=0;i<50000000;i++)
		p[i] = 1;
	for(;;);
    return 0;
}
