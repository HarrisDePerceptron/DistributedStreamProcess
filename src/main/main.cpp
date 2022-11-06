#include <iostream>
#include <vector>
#include <thread>
#include <fstream>
#include <future>
#include <sw/redis++/redis++.h>
#include "utilities.h"


#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ostream.h>


#include "task.h"

#include <unistd.h>

#include "hostinfo.h"

namespace RedisNS = sw::redis;

auto main(int argc, char *argv[]) -> int
{

	const std::string password = "87654321";
	const std::string host = "localhost";
	const int port = 6379;

	RedisNS::ConnectionOptions connopts;

	connopts.password = password;
	connopts.host = host;
	connopts.port = port;
	// connopts.socket_timeout = std::chrono::milliseconds(1000);

	RedisNS::Redis redis{connopts};

	// Attrs data = {{"language", "c#"}, {"loc", "500"}};

	// auto streamRes = redis.xadd("programming", "*", data.begin(),data.end());

	// std::cout<<"Got id: "<<streamRes<<std::endl;

	// std::vector<Item> output;

	// redis.xrange("programming","-","+", std::back_inserter(output));

	// for(const auto &e: output){
	// 	std::cout<<"timestamp: "<<e.first<<std::endl;

	// 	for(auto & attr: e.second.value()){
	// 		std::cout<<"key: "<<attr.first<<" value: "<<attr.second<<std::endl;

	// 	}

	// }


	

	std::string taskName = "task12";
	std::string dependentTask = "inputtask";

	Task task(redis, taskName, dependentTask);


	// const int HOSTNAME_MAX = 150;

	// char hostnameBuffer[HOSTNAME_MAX];
	// gethostname(hostnameBuffer, HOSTNAME_MAX);

	// char hostLoginBuffer[HOSTNAME_MAX];
	// getlogin_r(hostLoginBuffer, HOSTNAME_MAX);


	// std::string hostname = hostnameBuffer;
	// std::string hostLogin = hostLoginBuffer;

	// fmt::print("Your host name is: {}\nYour login name: {}\n", hostname, hostLogin);

	auto hostInfo = getHostInfo();


	fmt::print("Here is the info: {}", hostInfo);

	auto split = Utilities::splitString(hostInfo.hostName, '-');
	for(const auto & e: split){
		fmt::print("Token: {}\n", e);
		
	}
	


	return 0;
}