#include <iostream>
#include <vector>

#include <array>
#include <algorithm>



auto main(int argc, char *argv[]) -> int
{	

	std::vector<std::string>  args;
	auto begin = argv;

	std::transform(argv, argv+argc, std::back_inserter(args), [](char * s){
		return std::string{s};
	});

	for(const auto & e: args){
		std::cout<<e<<"\n";

	}


	return 0;
}