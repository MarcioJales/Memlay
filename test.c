
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
	res = sum(argc);
	prod(res, argc);

	return 0;
}
