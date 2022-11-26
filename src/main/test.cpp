

#include <iostream>
#include <vector>
#include <unordered_map>

#include <algorithm>

class Test
{

public:
	int i;
	Test()
	{
		std::cout << "Default constructor\n";
		i = 10;
	}

	Test(const Test &t)
	{
		std::cout << "Copy constructor called\n";
		i = t.i;
	}

	Test &operator=(const Test &t)
	{
		std::cout << "Copy assignment called\n";
		i = t.i;
		return *this;
	}

	Test(Test &&t)
	{
		std::cout << "Move constructor called\n";
		i = std::move(t.i);
		t.i = 0;
	}

	Test &operator=(Test &&t)
	{
		std::cout << "Move assignment called\n";
		i = std::move(t.i);

		t.i = 0;

		return *this;
	}

	~Test()
	{
		std::cout << "destructor\n";
	}
};

void fun(Test &t)
{
	std::cout << "In fun with non-const lvalue ref\n";
	std::vector<Test> v;
	v.push_back(std::move(t));

	std::cout << "i: " << v[0].i << "\n";
}

void fun2(const Test t)
{

	std::cout << "In fun2 with const lvalue\n";
}

void fun3(Test &t)
{
	std::cout << "In fun3 with const lvalue ref\n";
	std::vector<Test> v;
	v.push_back(std::move(t));

	std::cout << "i: " << v[0].i << "\n";
}

std::vector<Test> vv;
void fun3(Test &&t)
{
	std::cout << "In fun3 with rvalue ref\n";
	vv.push_back(std::move(t));

	std::cout << "i: " << vv[0].i << "\n";
	std::cout << "end fun3 with rvalue ref\n";
}

class TestClass
{

	int & ra;
	int a;


	public: 

	TestClass(int & _ra, int _a): ra{_ra}, a{_a}{

	}
	void constFun() const
	{
		ra = 20;
	}

	void constFun2() const
	{
		a = 30;
	}
};

int main(int argc, char *argv[])
{
	std::ios::sync_with_stdio(false);

	std::cout << "hello world" << std::endl;

	// std::vector<std::pair<std::string, std::string>> attrs;
	// attrs.push_back(
	// 	{"hello", "world"}
	// );
	// attrs.push_back(
	// 	{"hello", "world"}
	// );

	int a = 10;
	int &ra = a;

	std::cout << "a: " << a << "\n";
	
	TestClass tc {ra, a};

	tc.constFun();
	std::cout << "a: " << a << "\n";


	tc.constFun2();
	std::cout << "a: " << a << "\n";
	


	return 0;
}