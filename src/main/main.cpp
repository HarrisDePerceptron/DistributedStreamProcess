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

	std::string taskName = "task1";
	std::string dependentTask = "inputtask";
	std::string consumerName = "c2";

	// Task task(redis, taskName, dependentTask, consumerName);
	Task task(redis, taskName);

	TaskCallback callback = [](const DistributedTask::StreamMessage& msg){
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
			}
			return response;
	};

	// task.addCallback([](const DistributedTask::StreamMessage& msg){
	// 		fmt::print("recevied message2 {}\n", msg.messageId);
	// 		throw std::runtime_error{"Random error in callback"};
	// 		Attrs response;
	// 		return response;
	// });

	// task.addCallback(callback);

	// task.produce({
	// 	{"n1", "20"},
	// 	{"n2", "100"}
	// });	
	// task.consume(1);



	// task.sendMessage({
	// 	{"output", "5"}
	// });


	// # fetch messages in a stream

	// for (const auto & e: task.fetchErrorMessages(5)){
	// 	fmt::print("Stream xrange: {}: {}\n", e.streamName, e.messageId);

	// 	for (const auto &attr: e.data){
	// 		fmt::print("{}: {}\n", attr.first, attr.second);
	// 	}
	// }


	

	// for(const auto &e: task.fetchMessages(10)){
	// 	fmt::print("fetching: {}\n",e.messageId);
	// }


	// task.publish({
	// 	{"output", "20"}
	// });


	// TaskPublisher pub{task};


	// pub.publish({
	// 	{"n1", "1000"},
	// 	{"n2", "2000"}
	// });


	TaskConsumer consumer {task, consumerName};
	consumer.addCallback(callback);

	consumer.consume(1);

	return 0;
}