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
#include <spdlog/spdlog.h>

namespace RedisNS = sw::redis;
namespace logger = spdlog;

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
	std::string outputStreamName;
	std::string errorOutputStream;

	RedisNS::Redis &redis;

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

	std::string sendResultMessage(const Attrs &data)
	{

		auto messageId = sendResultMessage(data, "*");
		return messageId;
	}

	std::string sendResultMessage(const Attrs &data, std::string streamId)
	{

		auto messageId = sendMessage(outputStreamName, data, streamId);
		return messageId;
	}

	DistributedTask::StreamErrorMessage formatConsumerErrorMessage(const DistributedTask::StreamConsumerMessage &streamMessage, const std::string &message)
	{
		DistributedTask::StreamErrorMessage errorMessage;
		errorMessage.errorMessage = message;
		errorMessage.messageId = streamMessage.messageId;
		errorMessage.streamName = streamMessage.streamName;

		return errorMessage;
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
		sendMessage(errorOutputStream, serializedMessage, "*");
	}

	std::string getFieldValueFromAttributes(const Attrs &attributes, const std::string &fieldName) const
	{
		Attrs result;
		std::copy_if(attributes.begin(), attributes.end(), std::back_inserter(result), [&fieldName](const std::pair<std::string, std::string> &e)
					 {
			if (e.first==fieldName){
				return true;
			}	
			return false; });

		if (result.size() == 0)
		{
			throw std::runtime_error{fmt::format("Unable to find field '{}' in attributes", fieldName)};
		}

		auto value = std::move(result[0].second);
		return value;
	}

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
		this->outputStreamName = this->taskName + ":output";
		this->errorOutputStream = this->taskName + ":error";
	}

	Task(RedisNS::Redis &_redis, const std::string _taskName) : Task(_redis, _taskName, _taskName + ":input")
	{
	}

	std::string getErrorStream()
	{
		return errorOutputStream;
	}

	std::vector<DistributedTask::XInfoGroupResponse> getGroupInfo()
	{
		auto res = redis.command("xinfo", "groups", inputStreamName);
		auto xinfoRes = parseXInfoGroup(res);

		auto groupResponse = parseXinfoGroupResponse(xinfoRes);
		return groupResponse;
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
		auto result = fetchStreamhMessages(inputStreamName, startMessageId, endMessageId, count);
		return result;
	}

	std::vector<DistributedTask::StreamMessage> fetchOutputMessages(long long int count)
	{
		auto result = fetchOutputMessages("-", "+", count);
		return result;
	}

	std::vector<DistributedTask::StreamMessage> fetchOutputMessages(const std::string &startMessageId, long long int count)
	{
		auto result = fetchOutputMessages(startMessageId, "+", count);
		return result;
	}

	std::vector<DistributedTask::StreamMessage> fetchOutputMessages(const std::string &startMessageId, const std::string &endMessageId, long long int count)
	{
		auto result = fetchStreamhMessages(outputStreamName, startMessageId, endMessageId, count);
		return result;
	}

	std::vector<DistributedTask::StreamMessage> fetchErrorMessages(long long int count)
	{
		auto result = fetchErrorMessages("-", "+", count);
		return result;
	}

	std::vector<DistributedTask::StreamMessage> fetchErrorMessages(const std::string &startMessageId, long long int count)
	{
		auto result = fetchErrorMessages(startMessageId, "+", count);
		return result;
	}

	std::vector<DistributedTask::StreamMessage> fetchErrorMessages(const std::string &startMessageId, const std::string &endMessageId, long long int count)
	{
		auto result = fetchStreamhMessages(errorOutputStream, startMessageId, endMessageId, count);
		return result;
	}

	std::vector<DistributedTask::StreamMessage> fetchStreamhMessages(const std::string streamName, const std::string &startMessageId, const std::string &endMessageId, long long int count) const
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

	std::string getInputStreamName() const
	{
		return inputStreamName;
	}

	std::string getOutputStreamName() const
	{
		return outputStreamName;
	}

	std::string getErrorOutputStreamName() const
	{
		return errorOutputStream;
	}

	std::string sendMessage(const std::string &streamName, const Attrs &data, std::string streamId, const long long int maxLength)
	{

		return sendMessage(streamName, data,streamId, maxLength, true);
	}

	std::string sendMessage(const std::string &streamName, const Attrs &data, std::string streamId, const long long int maxLength, bool approximate)
	{

		if (maxLength != 0)
		{
			auto messageId = redis.xadd(streamName, streamId, data.begin(), data.end(), maxLength, approximate);
			return messageId;
		}

		auto messageId = redis.xadd(streamName, streamId, data.begin(), data.end());
		return messageId;
	}
	std::string sendMessage(const std::string &streamName, const Attrs &data, std::string streamId)
	{

		auto messageId = sendMessage(streamName, data, streamId, 0);
		return messageId;
	}

	std::string getTaskName() const
	{
		return taskName;
	}

	RedisNS::Redis &getRedis() const
	{
		return redis;
	}

	DistributedTask::StreamMessage getMessageById(const std::string &streamName, const std::string &messageId) const
	{
		auto messages = fetchStreamhMessages(streamName, messageId, messageId, 1);
		if (messages.size() == 0)
		{
			throw std::runtime_error{fmt::format("Message id {} not found", messageId)};
		}

		auto message = std::move(messages[0]);
		return message;
	}

	DistributedTask::StreamMessage getOutputById(const std::string &messageId) const
	{
		auto streamName = getOutputStreamName();
		auto message = getMessageById(streamName, messageId);
		return message;
	}

	DistributedTask::StreamMessage getErrorById(const std::string &messageId) const
	{
		auto streamName = getErrorOutputStreamName();
		auto message = getMessageById(streamName, messageId);
		return message;
	}

	DistributedTask::StreamMessage getStreamMessageFromError(const std::string &messageId) const
	{
		auto result = getErrorById(messageId);

		auto originalMessageId = getFieldValueFromAttributes(result.data, "messageId");
		auto originalStreamName = getFieldValueFromAttributes(result.data, "streamName");

		auto originalMessage = getMessageById(originalStreamName, originalMessageId);
		return originalMessage;
	}
};

// inputStreamName => last dependent task + _output
// groupName => Task Name
// Consumer Name => unique arbritarary consumer name ?  taskname + _consumer
// outputStreamName => taskname + _output
// errorStreamName
