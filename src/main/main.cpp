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
#include <variant>

namespace RedisNS = sw::redis;

using Attrs = std::vector<std::pair<std::string, std::string>>;
using Item = std::pair<std::string, RedisNS::Optional<Attrs>>;
using ItemStream = std::vector<Item>;

using GroupReadResult = std::unordered_map<std::string, ItemStream>;

struct StreamMessage
{
	std::string streamName;
	std::string messageId;
	std::string group;
	std::string consumer;
	Attrs data;

	friend std::ostream &operator<<(std::ostream &os, const StreamMessage &sm)
	{
		os << "{ ";
		os << "streamName: '" << sm.streamName << "', group: '" << sm.group << "', consumer: '" << sm.consumer
		   << "',\n"
		   << "messageId: '" << sm.messageId << "',\n";
		os << "{ ";
		for (const auto &e : sm.data)
		{
			os << e.first << ": " << e.second << ", ";
		}
		os << "}\n";

		return os;
	}
};

struct XInfoGroupResponse
{
	std::string groupName;
	std::string consumers;
	std::string pending;
	std::string lastDeliveredStreamId;
	std::string entriesRead;
	std::string lag;
};

class Task
{
private:
	std::string taskName;
	std::string dependentTask;
	std::string inputStreamName;
	std::string outputStreamName;
	std::string errorOutputStream;

	std::string groupName;

	std::string consumerName;

	RedisNS::Redis &redis;

	int totalRetries{3};

	using XinfoParseResponse = std::vector<std::vector<std::pair<std::string, std::string>>>;

	XinfoParseResponse parseXInfoGroup(const RedisNS::ReplyUPtr &xinfoReply)
	{
		XinfoParseResponse finalRespponse;

		if (xinfoReply->elements == 0)
		{
			return finalRespponse;
		}

		for (int groupCounter = 0; groupCounter < xinfoReply->elements; groupCounter++)
		{
			const auto &element = xinfoReply->element[groupCounter]->element;
			const unsigned int totalResponseFields = xinfoReply->element[groupCounter]->elements;

			std::vector<std::pair<std::string, std::string>> response;
			for (int i = 0; i < totalResponseFields; i += 2)
			{

				// const auto & name = element[i];
				const unsigned int keyIndex = i;
				const unsigned int valueIndex = i + 1;

				const auto &key = element[keyIndex];
				const auto &value = element[valueIndex];

				const std::string keyName = key->str;

				std::string val;

				if (RedisNS::reply::is_string(*value))
				{
					val = value->str;
				}
				else if (RedisNS::reply::is_integer(*value))
				{
					val = std::to_string(value->integer);
				}

				std::pair<std::string, std::string> res = {keyName, val};

				response.push_back(res);
			}

			finalRespponse.push_back(response);
		}

		return finalRespponse;
	}

	XinfoParseResponse parseXinfoGroupConsumer(const RedisNS::ReplyUPtr &consumerRes)
	{
		if (!RedisNS::reply::is_array(*consumerRes))
		{
			throw std::runtime_error{"Expected array response"};
		}

		const auto &consumerArray = consumerRes->element;
		const int totalConsumers = consumerRes->elements;

		XinfoParseResponse consumers;
		for (int c = 0; c < totalConsumers; c++)
		{
			const auto &consumer = consumerArray[c];

			if (!RedisNS::reply::is_array(*consumer))
			{
				throw std::runtime_error{"Key value must be an array"};
			}

			const int totalKeyValue = consumer->elements;

			std::vector<std::pair<std::string, std::string>> response;

			for (int i = 0; i < totalKeyValue; i += 2)
			{
				int keyIndex = i;
				int valueIndex = i + 1;

				std::string key = consumer->element[keyIndex]->str;

				const auto &valPtr = consumer->element[valueIndex];
				std::string value = "";

				if (RedisNS::reply::is_string(*valPtr))
				{
					value = valPtr->str;
				}
				else if (RedisNS::reply::is_integer(*valPtr))
				{
					value = std::to_string(valPtr->integer);
				}

				std::pair<std::string, std::string> res = {key, value};
				response.push_back(res);
			}

			consumers.push_back(response);
		}

		return consumers;
	}

public:
	Task() = delete;
	Task(const Task &) = delete;
	Task(const Task &&) = delete;
	Task &operator=(const Task &) = delete;
	Task &operator=(const Task &&) = delete;

	Task(RedisNS::Redis &_redis, const std::string _taskName, const std::string _dependentTask) : redis{_redis}
	{
		this->taskName = _taskName;
		this->dependentTask = _dependentTask;

		this->inputStreamName = this->dependentTask + "_output";
		this->outputStreamName = this->taskName + "_output";
		this->errorOutputStream = this->taskName + "_error";

		this->groupName = this->taskName;
		this->consumerName = this->taskName + "_consumer";
	}

	void sendOutput(const Attrs &data)
	{

		redis.xadd(outputStreamName, "*", data.begin(), data.end());
	}

	void sendOutput(const Attrs &data, std::string streamId)
	{

		redis.xadd(outputStreamName, streamId, data.begin(), data.end());
	}

	StreamMessage parseReadGroup(const GroupReadResult &result)
	{
		StreamMessage so;

		for (const auto &e : result)
		{
			so.streamName = e.first;

			for (const auto &si : e.second)
			{
				so.messageId = si.first;
				so.data = si.second.value();
			}
		}

		return so;
	}

	void ackknowledgeStreamMesssage(const std::string &streamId)
	{
		redis.xack(inputStreamName, groupName, streamId);
	}

	StreamMessage readNewGroupMessages(long long count, unsigned int waitDuration)
	{
		GroupReadResult result;
		std::chrono::milliseconds wait{waitDuration};

		try
		{
			redis.xreadgroup(groupName, consumerName, inputStreamName, ">", wait, count, std::inserter(result, result.end()));
		}
		catch (const std::exception &e)
		{
			fmt::print("Error: {}\n", e.what());
		}

		auto streamMessage = parseReadGroup(result);
		return streamMessage;
	}

	StreamMessage readPendingGroupMessages(long long count, unsigned int waitDurationMillis)
	{
		GroupReadResult result;
		std::chrono::milliseconds wait{waitDurationMillis};

		try
		{
			redis.xreadgroup(groupName, consumerName, inputStreamName, "0", wait, count, std::inserter(result, result.end()));
		}
		catch (const std::exception &e)
		{
			fmt::print("Error: {}\n", e.what());
		}

		auto streamMessage = parseReadGroup(result);
		return streamMessage;
	}

	void consume(long long count)
	{
		while (true)
		{
			GroupReadResult result;
			auto streamMessage = readNewGroupMessages(count, 500);

			fmt::print("Stream Message: \n {}", streamMessage);
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}

	XinfoParseResponse getGroupInfo()
	{
		auto res = redis.command("xinfo", "groups", inputStreamName);
		auto xinfoRes = parseXInfoGroup(res);
		return xinfoRes;
	}

	XinfoParseResponse getGroupConsumerInfo()
	{
		auto res = redis.command("xinfo", "consumers", inputStreamName, groupName);
		return res;
	}
};

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

	auto consumerRes = task.getGroupConsumerInfo();
	if (!RedisNS::reply::is_array(*consumerRes))
	{
		throw std::runtime_error{"Expected array response"};
	}

	const auto &consumerArray = consumerRes->element;
	const int totalConsumers = consumerRes->elements;

	std::vector<std::vector<std::pair<std::string, std::string>>> consumers;
	for (int c = 0; c < totalConsumers; c++)
	{
		const auto &consumer = consumerArray[c];

		if (!RedisNS::reply::is_array(*consumer))
		{
			throw std::runtime_error{"Key value must be an array"};
		}

		const int totalKeyValue = consumer->elements;

		std::vector<std::pair<std::string, std::string>> response;

		for (int i = 0; i < totalKeyValue; i += 2)
		{
			int keyIndex = i;
			int valueIndex = i + 1;

			std::string key = consumer->element[keyIndex]->str;

			const auto &valPtr = consumer->element[valueIndex];
			std::string value = "";

			if (RedisNS::reply::is_string(*valPtr))
			{
				value = valPtr->str;
			}
			else if (RedisNS::reply::is_integer(*valPtr))
			{
				value = std::to_string(valPtr->integer);
			}

			std::pair<std::string, std::string> res = {key, value};
			response.push_back(res);
		}

		consumers.push_back(response);
	}

	// std::transform(consumers.begin(), consumers.end(),)
	std::vector<std::string> names;
	std::transform(consumers.begin(), consumers.end(), std::back_inserter(names), [](const std::vector<std::pair<std::string, std::string>> &e)
				   { return e[0].second; });

	for (const auto &e : names)
	{
		fmt::print("name: {}\n", e);
	}
	fmt::print("Consumer res {}", RedisNS::reply::is_integer(*consumerRes));

	return 0;
}