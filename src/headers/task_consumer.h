#pragma once

#include <iostream>
#include <vector>
#include <thread>
#include <fmt/core.h>

#include "task_response.h"

#include <functional>

#include <spdlog/spdlog.h>

#include "task.h"
#include <optional>
#include "config.h"

namespace RedisNS = sw::redis;

namespace logger = spdlog;


class TaskConsumer
{
private:
	std::string groupName;
	std::string consumerName;
	
	std::vector<TaskCallback> callbacks;

	int totalRetries{3};
	std::chrono::milliseconds retryWait{1000};

	bool outputResult{true};
	bool outputError{true};
	
	unsigned long long outputMaxLength{0};
	unsigned long long errorMaxLength {0};
	

	Task &task;

	XinfoParseResponse parseXInfoGroup(const RedisNS::ReplyUPtr &xinfoReply)
	{
		XinfoParseResponse finalRespponse;

		if (xinfoReply->elements == 0)
		{
			return finalRespponse;
		}

		for (size_t groupCounter = 0; groupCounter < xinfoReply->elements; groupCounter++)
		{
			const auto &element = xinfoReply->element[groupCounter]->element;
			const size_t totalResponseFields = xinfoReply->element[groupCounter]->elements;

			std::vector<std::pair<std::string, std::string>> response;
			for (size_t i = 0; i < totalResponseFields; i += 2)
			{

				// const auto & name = element[i];
				const auto keyIndex = i;
				const auto valueIndex = i + 1;

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
		const auto totalConsumers = consumerRes->elements;

		XinfoParseResponse consumers;
		for (size_t c = 0; c < totalConsumers; c++)
		{
			const auto &consumer = consumerArray[c];

			if (!RedisNS::reply::is_array(*consumer))
			{
				throw std::runtime_error{"Key value must be an array"};
			}

			const auto totalKeyValue = consumer->elements;

			std::vector<std::pair<std::string, std::string>> response;

			for (size_t i = 0; i < totalKeyValue; i += 2)
			{
				auto keyIndex = i;
				auto valueIndex = i + 1;

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

	std::vector<DistributedTask::XInfoConsumer> parseXinfoConsumerResponse(const XinfoParseResponse &raw, const std::string &_groupName)
	{
		std::vector<DistributedTask::XInfoConsumer> response;

		for (const auto &groups : raw)
		{
			DistributedTask::XInfoConsumer res;
			res.groupName = _groupName;

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

	void ackknowledgeStreamMesssage(const std::string &streamId)
	{
		auto &redis = task.getRedis();
		auto inputStreamName = task.getInputStreamName();
		redis.xack(inputStreamName, groupName, streamId);
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
		auto &redis = task.getRedis();
		const auto &inputStreamName = task.getInputStreamName();
		try
		{
			redis.xreadgroup(groupName, consumerName, inputStreamName, ">", wait, count, std::inserter(result, result.end()));
		}
		catch (const std::exception &e)
		{
			logger::error("Error: {}\n", e.what());
		}

		auto streamMessage = parseReadGroup(result);
		return streamMessage;
	}

	std::vector<DistributedTask::StreamConsumerMessage> readPendingGroupMessages(long long count, unsigned int waitDurationMillis)
	{
		GroupReadResult result;
		std::chrono::milliseconds wait{waitDurationMillis};
		auto inputStreamName = task.getInputStreamName();
		auto &redis = task.getRedis();

		try
		{

			redis.xreadgroup(groupName, consumerName, inputStreamName, "0", wait, count, std::inserter(result, result.end()));
		}
		catch (const std::exception &e)
		{
			logger::error("[Error] [ReadPending] {}\n", e.what());
		}

		auto streamMessage = parseReadGroup(result);
		return streamMessage;
	}

	std::string sendResultMessage(const Attrs &data)
	{

		auto messageId = sendResultMessage(data, "*");
		return messageId;
	}

	std::string sendResultMessage(const Attrs &data, std::string streamId)
	{

		auto outputStreamName = task.getOutputStreamName();

		auto messageId = task.sendMessage(outputStreamName, data, streamId, outputMaxLength);
		return messageId;
	}

	void consumePending(long long count)
	{

		auto currentConsumerInfo = getCurrentConsumerInfo();
		if (currentConsumerInfo == nullptr)
		{
			logger::info("No consumer at the moment. wait for a message to received");
			return;
		}

		logger::info("Total pending messages: {}\n", currentConsumerInfo->pending);

		long long int totalBatches = currentConsumerInfo->pending / count;
		logger::info("Total pending batches: {}\n", totalBatches);

		while (true)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

			try
			{

				const auto pendingMessages = readPendingGroupMessages(count, 500);
				if (pendingMessages.size() == 0)
				{
					logger::info("No more pending messages...\n");
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
							logger::error("[Error] [ConsumePending] [Callback] {}\n", exxx.what());
						}

						ackknowledgeStreamMesssage(streamMessage.messageId);
					}
					catch (const std::exception &exx)
					{
						logger::error("[Error] [ConsumePending] [StreamMessage] {}\n", exx.what());
					}
				}
			}
			catch (const std::exception &ex)
			{
				logger::error("[Error] [ConsumePending] {}\n", ex.what());
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
		int currentTry = 0;

		const int totalTries = totalRetries + 1;

		while (currentTry < totalTries)
		{
			auto lastTry =  currentTry>=totalTries-1;		

			try
			{
				auto res = tcb(streamMessage);
				if (res.size() > 0)
				{
					if(outputResult){
						sendResultMessage(res);
					}
				}

				break;
			}
			catch (const std::exception &exxx)
			{
				const auto &message = exxx.what();
				logger::error("[ERROR] [CONSUMER] [Callback] {}\n", message);
				
				if (outputError){
					if(lastTry){
						auto errorMessage = formatConsumerErrorMessage(streamMessage, message);
						sendErrorMessage(errorMessage);
					}
				}
			}

			
			currentTry++;
			
			if(currentTry < totalTries){
				logger::info("Retrying... Current try: {}\n", currentTry);
				std::this_thread::sleep_for(retryWait);
			}
			
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

				ackknowledgeStreamMesssage(streamMessage.messageId);
			}
			catch (const std::exception &ex)
			{
				logger::error("[ERROR] [CONSUMER] [StreamMessage] {}\n", ex.what());
			}
		}
	}

	Attrs serializeErrorMessage(const DistributedTask::StreamErrorMessage &err)
	{
		Attrs attrs = {
			{"errorMessage", err.errorMessage},
			{"messageId", err.messageId},
			{"streamName", err.streamName}};

		return attrs;
	}

	void sendErrorMessage(const DistributedTask::StreamErrorMessage &err)
	{
		auto serializedMessage = serializeErrorMessage(err);
		auto errorOutputStream = task.getErrorOutputStreamName();
		task.sendMessage(errorOutputStream, serializedMessage, "*", errorMaxLength);
	}

public:
	TaskConsumer() = delete;
	TaskConsumer(Task &&) = delete;
	TaskConsumer &operator=(const Task &) = delete;
	TaskConsumer &operator=(Task &&) = delete;

	TaskConsumer(Task &_task, const std::string _consumerName) : task{_task}
	{

		this->groupName = task.getTaskName();
		this->consumerName = _consumerName;

		initialize();
	}

	TaskConsumer(Task &_task, const std::string _consumerName, const DistributedTask::ConsumerConfig & config) : TaskConsumer{_task, _consumerName}
	{

		logger::info("Consumer received config: {}\n", config);

		if (config.retryWait){
			this->retryWait =  *config.retryWait;
		}

		if (config.totalRetries){
			this->totalRetries =  *config.totalRetries;
		}

		if (config.outputResult){
			this->outputResult = *config.outputResult;
		}

		if (config.outputError){
			this->outputError = *config.outputError;
		}
		if (config.outputMaxLength){
			this->outputMaxLength = *config.outputMaxLength;
		}
		
		if (config.errorMaxLength){
			this->errorMaxLength = *config.errorMaxLength;
		}

	}

	void consume(long long count)
	{
		consumePending(count);

		while (true)
		{
			// std::this_thread::sleep_for(std::chrono::milliseconds(1));

			try
			{
				auto streamMessages = readNewGroupMessages(count, 500);
				consumeMessages(streamMessages);
			}
			catch (const std::exception &e)
			{
				logger::info("[ERROR] [CONSUME] {}\n", e.what());
			}
		}
	}

	std::vector<DistributedTask::XInfoGroupResponse> getGroupInfo()
	{
		auto &redis = task.getRedis();
		auto inputStreamName = task.getInputStreamName();
		auto res = redis.command("xinfo", "groups", inputStreamName);
		auto xinfoRes = parseXInfoGroup(res);

		auto groupResponse = parseXinfoGroupResponse(xinfoRes);
		return groupResponse;
	}

	std::vector<DistributedTask::XInfoConsumer> getGroupConsumerInfo()
	{
		auto &redis = task.getRedis();
		auto inputStreamName = task.getInputStreamName();
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
		auto taskName = task.getTaskName();
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
		logger::info("Consumer name is {}\n", this->consumerName);

		auto &redis = task.getRedis();
		auto inputStreamName = task.getInputStreamName();
		auto taskName = task.getTaskName();

		if (!task.streamExists(inputStreamName))
		{
			logger::info("Task {} does not exists\n", taskName);
			redis.xgroup_create(inputStreamName, groupName, "$", true);
		}
		else if (!groupExists())
		{
			logger::info("Group {} does not exists\n", groupName);
			redis.xgroup_create(inputStreamName, groupName, "$");
		}

		auto consumerInfo = getGroupConsumerInfo();
		logger::info("Total consumers: {}\n", consumerInfo.size());
	}
};
