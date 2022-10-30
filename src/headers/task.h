#pragma once


#include <iostream>
#include <vector>
#include <thread>
#include <sw/redis++/redis++.h>
#include "utilities.h"
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ostream.h>


#include "task_response.h"


namespace RedisNS = sw::redis;

using Attrs = std::vector<std::pair<std::string, std::string>>;
using Item = std::pair<std::string, RedisNS::Optional<Attrs>>;
using ItemStream = std::vector<Item>;

using GroupReadResult = std::unordered_map<std::string, ItemStream>;


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
		auto xinfoconsumerResponse = parseXinfoGroupConsumer(res);
		return xinfoconsumerResponse;
	}
};