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

#include <type_traits>

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
	std::string consumerName = "c2";

	Task task(redis, taskName, dependentTask, consumerName );

	TaskCallback callback = [](const DistributedTask::StreamMessage& msg){
			fmt::print("recevied message {}\n", msg.messageId);
			
	};


	task.addCallback(callback);


	task.addCallback([](const DistributedTask::StreamMessage& msg){
			fmt::print("recevied message2 {}\n", msg.messageId);
			// throw std::runtime_error{"Random error in callback"};			
	});
	
	task.consume(1);
	



	return 0;
}