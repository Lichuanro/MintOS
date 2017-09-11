#include "type.h"
#include "stdio.h"
#include "string.h"

int main(int argc, char * argv[])
{
	char rdbuf[128];
	printf("please enter username:\n");
	int r = read(0, rdbuf, 70);
	rdbuf[r] = 0;
	printf("%s\n", rdbuf);
	return 0;
}
