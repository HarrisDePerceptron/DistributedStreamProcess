#pragma once

#include <iostream>
#include <vector>
#include <thread>
#include <sw/redis++/redis++.h>
#include "utilities.h"
#include <fmt/core.h>
#include <fmt/ostream.h>

#include "task_response.h"
#include "hostinfo.h"

#include <functional>

namespace RedisNS = sw::redis;

using Attrs = std::vector<std::pair<std::string, std::string>>;
using Item = std::pair<std::string, RedisNS::Optional<Attrs>>;
using ItemStream = std::vector<Item>;

using GroupReadResult = std::unordered_map<std::string, ItemStream>;

using TaskCallback = std::function<Attrs(const DistributedTask::StreamMessage &)>;
using XinfoParseResponse = std::vector<std::vector<std::pair<std::string, std::string>>>;

class Task
{
private:
	std::string taskName;
	std::string dependentTask;
	std::string inputStreamName;

	RedisNS::Redis &redis;

public:
	Task() = delete;
	Task(const Task &&) = delete;
	Task &operator=(const Task &) = delete;
	Task &operator=(const Task &&) = delete;

	Task(RedisNS::Redis &_redis, const std::string _taskName, const std::string _dependentTask) : redis{_redis}
	{
		this->taskName = _taskName;
		this->dependentTask = _dependentTask;
		this->inputStreamName = this->dependentTask + ":output";

		initialize();
	}

	Task(RedisNS::Redis &_redis, const std::string _taskName): 
		Task(redis, _taskName,  _taskName + ":input")
	{		

	}

	bool streamExists(const std::string &streamName)
	{

		try
		{
			auto res = redis.type(streamName);
			if (res != "stream")
			{
				fmt::print("input key is not a stream but '{}'\n", res);
				return false;
			}
			else
			{
				return true;
			}
		}
		catch (const std::exception &e)
		{
			fmt::print("Exception at redis type: {}", e.what());
		}

		return false;
	}
	void initialize()
	{
	}

	std::vector<DistributedTask::StreamMessage> fetchMessages(long long int count)
	{

		auto result = fetchMessages("-", "+", count);
		return result;
	}

	std::vector<DistributedTask::StreamMessage> fetchMessages(const std::string &startMessageId, long long int count)
	{

		auto result = fetchMessages(startMessageId, "+", count);
		return result;
	}

	std::vector<DistributedTask::StreamMessage> fetchMessages(const std::string &startMessageId, const std::string &endMessageId, long long int count)
	{
		auto result = fetcStreamhMessages(inputStreamName, startMessageId, endMessageId, count);
		return result;
	}

	std::vector<DistributedTask::StreamMessage> fetcStreamhMessages(const std::string streamName, const std::string &startMessageId, const std::string &endMessageId, long long int count)
	{

		ItemStream is;
		redis.xrange(streamName, startMessageId, endMessageId, count, std::back_inserter(is));

		std::vector<DistributedTask::StreamMessage> messages;
		for (const auto &e : is)
		{

			const auto &messageId = e.first;
			if (!e.second)
			{
				continue;
			}

			DistributedTask::StreamMessage message;

			const auto &attrs = *e.second;

			message.messageId = std::move(messageId);
			message.streamName = streamName;

			message.data = std::move(attrs);
			messages.push_back(std::move(message));
		}
		return messages;
	}

	void publish(const Attrs & values){
		publish(values, "*");
	}
	void publish(const Attrs &values,  const std::string &streamId){
		sendMessage(inputStreamName, values, streamId);

	}

	std::string sendMessage(const std::string &streamName, const Attrs &data, std::string streamId)
	{

		auto messageId = redis.xadd(streamName, streamId, data.begin(), data.end());
		return messageId;
	}

};

// inputStreamName => last dependent task + _output
// groupName => Task Name
// Consumer Name => unique arbritarary consumer name ?  taskname + _consumer
// outputStreamName => taskname + _output
// errorStreamName
