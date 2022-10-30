#pragma once

#include <string>
#include <vector>
#include <iostream>


using Attrs = std::vector<std::pair<std::string, std::string>>;

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
