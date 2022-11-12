

#include <iostream>
#include <vector>
#include <unordered_map>

#include <algorithm>

bool is_number(const std::string &s)
{

	auto begin = s.begin();
	if (*begin == '-')
	{
		begin++;
	}

	return !s.empty() && std::find_if(begin,
									  s.end(), [](unsigned char c)
									  { return !std::isdigit(c); }) == s.end();
}


int main(int argc, char *argv[])
{
	std::ios::sync_with_stdio(false);

	std::cout << "hello world" << std::endl;

	std::string s = "123.0";

	std::cout<<is_number(s)<<"\n";


	return 0;
}