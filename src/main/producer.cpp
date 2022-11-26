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


	std::vector<std::string> args;
	std::transform(argv+1, argv+argc, std::back_inserter(args),[](const char * str){
		return std::string {str};		
	});


	if (args.size()<2){
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
	std::string dependentTask = "inputtask";
	std::string consumerName = "c1";

	Task task(redis, taskName, dependentTask, consumerName );


	task.produce({
		{"n1", args[0]},
		{"n2", args[1]}
	});

	
	return 0;
}