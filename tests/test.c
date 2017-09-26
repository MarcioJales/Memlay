#include <unistd.h>
#include <stdlib.h>

int sum(int c)
{
	int count = 0, s = 0;
	while(count < 10) {
		s = s + c;
		count++;
	}
	return s;
};

void prod(int s, int c)
{
	int count = 0;
	while(count < 12){
		s = s*c;
		count++;
	}
};

int main(char** argv, int argc)
{
	int res;
	int *t = malloc(65536);
	res = sum(argc);
	int *r = malloc(65536);
	prod(res, argc);

	return 0;
}
