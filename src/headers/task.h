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

	std::vector<DistributedTask::XInfoConsumer> parseXinfoConsumerResponse(const XinfoParseResponse &raw, const std::string & groupName)
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

public:
	Task() = delete;
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

		initialize();
	}

	void sendOutput(const Attrs &data)
	{

		redis.xadd(outputStreamName, "*", data.begin(), data.end());
	}

	void sendOutput(const Attrs &data, std::string streamId)
	{

		redis.xadd(outputStreamName, streamId, data.begin(), data.end());
	}

	DistributedTask::StreamMessage parseReadGroup(const GroupReadResult &result)
	{
		DistributedTask::StreamMessage so;

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

	DistributedTask::StreamMessage readNewGroupMessages(long long count, unsigned int waitDuration)
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

	DistributedTask::StreamMessage readPendingGroupMessages(long long count, unsigned int waitDurationMillis)
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

	bool consumerExists(){
		auto consumerInfo = getGroupConsumerInfo();
		bool exists = false;

		for(const auto & e: consumerInfo){
			if (e.consumerName== consumerName ){
				exists = true;
				
			}
		}
		return exists;
	}

	bool groupExists(){
		auto consumerInfo = getGroupInfo();

		bool exists = false;

		for(const auto & e: consumerInfo){
			if (e.groupName== groupName ){
				exists = true;
				
			}
		}
		return exists;
	}



	bool streamExists(const std::string & streamName){

		try{
			auto res = redis.type(streamName);
			if(res!="stream"){
				fmt::print("input key is not a stream but '{}'\n", res);
				return false;
			}else{
				return true;
			}
		}catch(const std::exception & e){
			fmt::print("Exception at redis type: {}", e.what());
		}

		return false;
	}

	std::string getConsumeName(){
		std::string consumerName = taskName;
		auto hostInfo = getHostInfo();
		
	
	}



	void initialize(){
		
		if (!streamExists(inputStreamName)){
			fmt::print("Task {} does not exists\n", taskName);
			redis.xgroup_create(inputStreamName, groupName, "$",true);
		}else if (!groupExists()){
			fmt::print("Group {} does not exists\n", groupName);
			redis.xgroup_create(inputStreamName, groupName, "$");
		}

	}
};


// inputStreamName => last dependent task + _output
// groupName => Task Name
// Consumer Name => unique arbritarary consumer name ?  taskname + _consumer
// outputStreamName => taskname + _output
// errorStreamName 
