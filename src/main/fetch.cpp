#include <iostream>
#include <vector>
#include <thread>
#include <sw/redis++/redis++.h>


#include <fmt/core.h>

#include "task.h"

#include <unistd.h>

#include <type_traits>


namespace RedisNS = sw::redis;

auto main(int argc, char *argv[]) -> int
{


	std::vector<std::string> args;
	std::transform(argv+1, argv+argc, std::back_inserter(args),[](const char * str){
		return std::string {str};		
	});


	if (args.size()<1){
		fmt::print("Please provide atleast one message id\n");
		return 0;
	}


	const std::string password = "87654321";
	const std::string host = "localhost";
	const int port = 6379;

	RedisNS::ConnectionOptions connopts;

	connopts.password = password;
	connopts.host = host;
	connopts.port = port;
	// connopts.socket_timeout = std::chrono::milliseconds(1000);

	RedisNS::Redis redis{connopts};

	std::string taskName = "task1";

	Task task(redis, taskName);


	for (const auto & arg: args){
		fmt::print("Arg: {}\n", arg);

		fmt::print("Output stream name: {}\n", task.getOutputStreamName());

		for(const auto & e: task.fetchOutputMessages(arg,10)){

			fmt::print("{}\n", e.messageId);
			for(const auto & attr: e.data){
				fmt::print("attr: {}: {}\n",attr.first, attr.second);
			}
		}
		
	}

	return 0;
}