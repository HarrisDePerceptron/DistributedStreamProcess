


#include <iostream>
#include <vector>
#include <unordered_map>


int main(int argc, char *argv[]){
	std::ios::sync_with_stdio(false);

	std::cout<<"hello world"<<std::endl;


	std::vector<std::string> names = {"hamza", "haider", "hassan"};
	
	std::vector<std::string>::iterator it;
	
	for(it=names.begin(); it!=names.end();it++){
		std::cout<<*it<<std::endl;

	}


	std::unordered_map<std::string, int> ages = {
		{"harris", 28},
		{"hamza", 24},
		{"hassan", 18}
	};

	std::unordered_map<std::string, int>::iterator itO;
	for(itO=ages.begin();itO!=ages.end();itO++){
		std::cout<<itO->first<<": "<<itO->second<<std::endl;

	}

	

	return 0;
}