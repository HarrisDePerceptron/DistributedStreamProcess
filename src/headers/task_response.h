#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

using Attrs = std::vector<std::pair<std::string, std::string>>;

namespace DistributedTask
{
	struct StreamMessage
	{
		std::string streamName;
		std::string messageId;
		Attrs data;

		friend std::ostream &operator<<(std::ostream &os, const StreamMessage &sm)
		{
			os << "{ ";
			os << "streamName: '" << sm.streamName << "', group: '"
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


	struct StreamErrorMessage
	{
		std::string streamName;
		std::string messageId;
		std::string errorMessage;
		
	};


	struct StreamConsumerMessage: public StreamMessage
	{
		std::string group;
		std::string consumer;

		friend std::ostream &operator<<(std::ostream &os, const StreamConsumerMessage &sm)
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
		unsigned int consumers=0;
		unsigned int pending=0;
		std::string lastDeliveredStreamId;
		unsigned int entriesRead=0;
		unsigned int lag=0;

		friend std::ostream & operator<<(std::ostream & os,  const XInfoGroupResponse & res){

			os<<"{\n";
			os<<"groupName: "<<res.groupName<<",";
			os<<"\n"<<"consumers: "<<res.consumers<<",";
			os<<"\n"<<"pending: "<<res.pending<<",";
			os<<"\n"<<"lastDeliveredStreamId: "<<res.lastDeliveredStreamId<<",";
			os<<"\n"<<"entriesRead: "<<res.entriesRead<<",";
			os<<"\n"<<"lag: "<<res.lag;
			os<<"\n}";
			return os;

		}
	};




	struct XInfoConsumer
	{
		std::string groupName;
		std::string consumerName;
		unsigned int pending;
		long long int idle;

		friend std::ostream & operator<<(std::ostream & os,  const XInfoConsumer & res){

			os<<"{\n";
			os<<"groupName: "<<res.groupName<<",";
			os<<"\n"<<"consumerName: "<<res.consumerName<<",";
			os<<"\n"<<"pending: "<<res.pending<<",";
			os<<"\n"<<"idle: "<<res.idle;
			os<<"\n}";
			return os;

		}
	};


}
