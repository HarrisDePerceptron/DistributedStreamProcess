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

	std::string taskName = "task1";
	std::string dependentTask = "inputtask";

	Task task(redis, taskName, dependentTask);

	// task.consume(1);
	auto res = task.getGroupInfo();

	for (const auto &e : res)
	{
		for (const auto &i : e)
		{
			fmt::print("{}: {}\n", i.first, i.second);
		}
	}

	auto consumers = task.getGroupConsumerInfo();
	

	// std::transform(consumers.begin(), consumers.end(),)
	std::vector<std::string> names;
	std::transform(consumers.begin(), consumers.end(), std::back_inserter(names), [](const std::vector<std::pair<std::string, std::string>> &e)
				   { return e[0].second; });

	for (const auto &e : names)
	{
		fmt::print("name: {}\n", e);
	}

	return 0;
}