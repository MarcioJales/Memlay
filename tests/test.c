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

int main(int argc, char** argv)
{
	int res;
	int *t = (int *) malloc(2);
	res = sum(argc);
	int *r = (int *) malloc(300000);
	prod(res, argc);

	return 0;
}
