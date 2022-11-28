#pragma once

#include <iostream>
#include <vector>
#include <thread>
#include <sw/redis++/redis++.h>
#include "utilities.h"
#include <fmt/core.h>
#include <fmt/ostream.h>

#include "task_response.h"
#include "task"


namespace RedisNS = sw::redis;

using Attrs = std::vector<std::pair<std::string, std::string>>;
using Item = std::pair<std::string, RedisNS::Optional<Attrs>>;
using ItemStream = std::vector<Item>;

using GroupReadResult = std::unordered_map<std::string, ItemStream>;

using TaskCallback = std::function<Attrs(const DistributedTask::StreamMessage &)>;

class TaskConsumer
{
private:
	std::string dependentTask;
	std::string inputStreamName;

	std::string outputStreamName;
	std::string errorOutputStream;

	std::string groupName;
	std::string consumerName;

	std::vector<TaskCallback> callbacks;

	int totalRetries{3};
	bool outputResult{true};

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

	std::vector<DistributedTask::XInfoGroupResponse> parseXinfoGroupResponse(const XinfoParseResponse &raw)
	{
		std::vector<DistributedTask::XInfoGroupResponse> response;

		for (const auto &groups : raw)
		{
			DistributedTask::XInfoGroupResponse res;

			for (const auto &attr : groups)
			{
				const auto &key = attr.first;
				const auto &value = attr.second;

				std::string kl = "";

				std::transform(key.begin(), key.end(), std::back_inserter(kl), [](const char &c)
							   { return std::tolower(c); });

				if (kl == "name")
				{
					res.groupName = value;
				}
				else if (kl == "consumers")
				{
					if (value != "")
						res.consumers = std::stoi(value);
				}
				else if (kl == "pending")
				{
					if (value != "")
						res.pending = std::stoi(value);
				}
				else if (kl == "last-delivered-id")
				{
					res.lastDeliveredStreamId = value;
				}
				else if (kl == "entries-read")
				{
					if (value != "")
						res.entriesRead = std::stoi(value);
				}
				else if (kl == "lag")
				{
					if (value != "")
						res.lag = std::stoi(value);
				}
			}

			response.push_back(res);
		}

		return response;
	}

	std::vector<DistributedTask::XInfoConsumer> parseXinfoConsumerResponse(const XinfoParseResponse &raw, const std::string &groupName)
	{
		std::vector<DistributedTask::XInfoConsumer> response;

		for (const auto &groups : raw)
		{
			DistributedTask::XInfoConsumer res;
			res.groupName = groupName;

			for (const auto &attr : groups)
			{
				const auto &key = attr.first;
				const auto &value = attr.second;

				std::string kl = "";

				std::transform(key.begin(), key.end(), std::back_inserter(kl), [](const char &c)
							   { return std::tolower(c); });

				if (kl == "name")
				{
					res.consumerName = value;
				}
				else if (kl == "pending")
				{
					if (value != "")
						res.pending = std::stoi(value);
				}
				else if (kl == "idle")
				{
					if (value != "")
						res.idle = std::stoi(value);
				}
			}
			response.push_back(res);
		}

		return response;
	}

	std::vector<DistributedTask::StreamConsumerMessage> parseReadGroup(const GroupReadResult &result)
	{
		std::vector<DistributedTask::StreamConsumerMessage> streamMessages;

		for (const auto &e : result)
		{
			DistributedTask::StreamConsumerMessage so;
			so.group = groupName;
			so.consumer = consumerName;
			so.streamName = e.first;

			for (const auto &si : e.second)
			{
				so.messageId = si.first;
				so.data = si.second.value();
			}

			if (so.messageId != "")
			{
				streamMessages.push_back(so);
			}
		}

		return streamMessages;
	}

	std::vector<DistributedTask::StreamConsumerMessage> readNewGroupMessages(long long count, unsigned int waitDuration)
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

	std::vector<DistributedTask::StreamConsumerMessage> readPendingGroupMessages(long long count, unsigned int waitDurationMillis)
	{
		GroupReadResult result;
		std::chrono::milliseconds wait{waitDurationMillis};

		try
		{

			redis.xreadgroup(groupName, consumerName, inputStreamName, "0", wait, count, std::inserter(result, result.end()));
		}
		catch (const std::exception &e)
		{
			fmt::print("[Error] [ReadPending] {}\n", e.what());
		}

		auto streamMessage = parseReadGroup(result);
		return streamMessage;
	}


	void consumePending(long long count)
	{

		auto currentConsumerInfo = getCurrentConsumerInfo();
		if (currentConsumerInfo == nullptr)
		{
			fmt::print("No consumer at the moment. wait for a message to received");
			return;
		}

		fmt::print("Total pending messages: {}\n", currentConsumerInfo->pending);

		long long int totalBatches = currentConsumerInfo->pending / count;
		fmt::print("Total pending batches: {}\n", totalBatches);

		while (true)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

			try
			{

				const auto pendingMessages = readPendingGroupMessages(count, 500);
				if (pendingMessages.size() == 0)
				{
					fmt::print("No more pending messages...\n");
					break;
				}
				for (const auto &streamMessage : pendingMessages)
				{
					try
					{

						try
						{
							for (const auto &e : callbacks)
							{
								e(streamMessage);
							}
						}
						catch (const std::exception &exxx)
						{
							fmt::print("[Error] [ConsumePending] [Callback] {}\n", exxx.what());
						}

						ackknowledgeStreamMesssage(streamMessage.messageId);
					}
					catch (const std::exception &exx)
					{
						fmt::print("[Error] [ConsumePending] [StreamMessage] {}\n", exx.what());
					}
				}
			}
			catch (const std::exception &ex)
			{
				fmt::print("[Error] [ConsumePending] {}\n", ex.what());
			}
		}
	}

	DistributedTask::StreamErrorMessage formatConsumerErrorMessage(const DistributedTask::StreamConsumerMessage &streamMessage, const std::string &message)
	{
		DistributedTask::StreamErrorMessage errorMessage;
		errorMessage.errorMessage = message;
		errorMessage.messageId = streamMessage.messageId;
		errorMessage.streamName = streamMessage.streamName;

		return errorMessage;
	}
	void consumeMessage(const TaskCallback &tcb, const DistributedTask::StreamConsumerMessage &streamMessage)
	{

		try
		{
			auto res = tcb(streamMessage);
			if (res.size() > 0)
			{
				sendResultMessage(res);
			}
		}
		catch (const std::exception &exxx)
		{
			const auto &message = exxx.what();
			auto errorMessage = formatConsumerErrorMessage(streamMessage, message);
			sendErrorMessage(errorMessage);

			fmt::print("[ERROR] [CONSUMER] [Callback] {}\n", message);
		}
	}

	void consumeMessages(const std::vector<DistributedTask::StreamConsumerMessage> &streamMessages)
	{
		for (const auto &streamMessage : streamMessages)
		{

			try
			{
				if (streamMessage.messageId == "")
				{
					continue;
				}

				for (const auto &e : callbacks)
				{
					consumeMessage(e, streamMessage);
				}

				fmt::print("Stream Message: \n {}", streamMessage);
				ackknowledgeStreamMesssage(streamMessage.messageId);
			}
			catch (const std::exception &ex)
			{
				fmt::print("[ERROR] [CONSUMER] [StreamMessage] {}\n", ex.what());
			}
		}
	}


public:
	TaskConsumer() = delete;
	TaskConsumer(Task &&) = delete;
	TaskConsumer &operator=(const Task &) = delete;
	TaskConsumer &operator=(TaskConsumer &&) = delete;

	TaskConsumer(, const std::string _consumerName)
	{
		this->taskName = _taskName;
		this->dependentTask = _dependentTask;

		this->inputStreamName = this->dependentTask + ":output";
		this->outputStreamName = this->taskName + ":output";
		this->errorOutputStream = this->taskName + ":error";

		this->groupName = this->taskName;

		this->consumerName = _consumerName;

		initialize();
	}

	Task(RedisNS::Redis &_redis, const std::string _taskName, const std::string _consumerName): 
		Task(redis, _taskName,  _taskName + ":input", _consumerName)
	{		

	}

	std::string getErrorStream()
	{
		return errorOutputStream;
	}

	void consume(long long count)
	{
		consumePending(count);

		while (true)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			try
			{
				auto streamMessages = readNewGroupMessages(count, 500);
				consumeMessages(streamMessages);
			}
			catch (const std::exception &e)
			{
				fmt::print("[ERROR] [CONSUME] {}\n", e.what());
			}
		}
	}

	std::vector<DistributedTask::XInfoGroupResponse> getGroupInfo()
	{
		auto res = redis.command("xinfo", "groups", inputStreamName);
		auto xinfoRes = parseXInfoGroup(res);

		auto groupResponse = parseXinfoGroupResponse(xinfoRes);
		return groupResponse;
	}

	std::vector<DistributedTask::XInfoConsumer> getGroupConsumerInfo()
	{
		auto res = redis.command("xinfo", "consumers", inputStreamName, groupName);
		auto xinfoconsumerParseResponse = parseXinfoGroupConsumer(res);
		auto xinfoConsumerResponse = parseXinfoConsumerResponse(xinfoconsumerParseResponse, groupName);
		return xinfoConsumerResponse;
	}

	bool consumerExists()
	{
		auto consumerInfo = getGroupConsumerInfo();
		bool exists = false;

		for (const auto &e : consumerInfo)
		{
			if (e.consumerName == consumerName)
			{
				exists = true;
			}
		}
		return exists;
	}

	bool groupExists()
	{
		auto consumerInfo = getGroupInfo();

		bool exists = false;

		for (const auto &e : consumerInfo)
		{
			if (e.groupName == groupName)
			{
				exists = true;
			}
		}
		return exists;
	}

	std::string getConsumerName()
	{
		std::string finalConsumerName = taskName;
		auto hostInfo = getHostInfo();
		finalConsumerName += "_" + hostInfo.hostName;
		finalConsumerName += "_" + this->consumerName;
		return finalConsumerName;
	}

	std::unique_ptr<DistributedTask::XInfoConsumer> getCurrentConsumerInfo()
	{
		auto consumerInfos = getGroupConsumerInfo();
		for (const auto &e : consumerInfos)
		{
			if (e.consumerName == consumerName)
			{
				auto response = std::make_unique<DistributedTask::XInfoConsumer>(e);
				return response;
			}
		}

		return nullptr;
	}

	void addCallback(TaskCallback &callback)
	{
		callbacks.push_back(std::move(callback));
	}

	void addCallback(TaskCallback &&callback)
	{
		callbacks.push_back(callback);
	}

	void initialize()
	{

		this->consumerName = getConsumerName();
		fmt::print("Consumer name is {}\n", this->consumerName);

		if (!streamExists(inputStreamName))
		{
			fmt::print("Task {} does not exists\n", taskName);
			redis.xgroup_create(inputStreamName, groupName, "$", true);
		}
		else if (!groupExists())
		{
			fmt::print("Group {} does not exists\n", groupName);
			redis.xgroup_create(inputStreamName, groupName, "$");
		}

		auto consumerInfo = getGroupConsumerInfo();
		fmt::print("Total consumers: {}\n", consumerInfo.size());
	}

};
