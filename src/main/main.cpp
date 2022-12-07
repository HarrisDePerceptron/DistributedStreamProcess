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
#include "task_publisher.h"
#include "task_consumer.h"

#include "config.h"

namespace RedisNS = sw::redis;
namespace logger = spdlog;


auto main(int , char *[]) -> int
{

	logger::set_level(logger::level::debug);

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
	std::string consumerName = "c2";

	// Task task(redis, taskName, dependentTask, consumerName);
	Task task(redis, taskName);



	int count {0};

	TaskCallback callback = [&count](const DistributedTask::StreamMessage& msg){
			fmt::print("recevied message {}\n", msg.messageId);
			std::vector<std::pair<std::string, std::string>> res;
			std::copy_if(msg.data.begin(), msg.data.end(), std::back_inserter(res), [](const std::pair<std::string, std::string> & kv){
					if(kv.first == "n1"){
						return true;
					}

					if(kv.first == "n2"){
						return true;
					}

					return false;					
			});
			
			Attrs response;

			if (res.size()>= 2){

				int a = std::stoi(res[0].second);
				int b = std::stoi(res[1].second);
				
				auto sum = a+b;
				
				std::string output = std::to_string(sum);

				response.push_back(
					{"output", output}
				);
				
				logger::debug("Output: {}\n", output);
			}
			

			count++;
			logger::debug("Count callback: {}", count);
			return response;
	};


	DistributedTask::ConsumerConfig  cc;

	cc.totalRetries = 0;
	cc.retryWait = std::chrono::milliseconds{10};
	cc.outputResult = true;
	cc.outputError = true;
	cc.outputMaxLength = 5;
	cc.errorMaxLength = 5;


	fmt::print("Consumer config: {}\n", cc);

	TaskConsumer consumer {task, consumerName, cc};
	consumer.addCallback(callback);

	// consumer.addCallback([](const DistributedTask::StreamMessage & )->Attrs{
	// 		throw std::runtime_error{"Random exception generated"};
			
	// });

	consumer.consume(1);



	// auto result = task.getStreamMessageFromError("1669988579395-0");

	// for(const auto& e: result.data){
	// 	logger::debug("Original {}: {}", e.first, e.second);
	// }

	return 0;
}