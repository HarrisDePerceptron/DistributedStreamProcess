#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <optional>

namespace DistributedTask
{
	struct ConsumerConfig
	{

		std::optional<unsigned int> totalRetries;
		std::optional<std::chrono::milliseconds> retryWait;
		std::optional<bool> outputResult;
		std::optional<bool> outputError;
	};

	std::ostream &operator<<(std::ostream &os, const ConsumerConfig &cc){
			os<<"{\n";
			if(cc.totalRetries)
			os<<"\t"<<"totalRetries: "<<*cc.totalRetries;

			if(cc.retryWait)
			os<<",\n\t"<<"retryWait: "<<cc.retryWait.value().count();

			if(cc.outputResult)
			os<<",\n\t"<<"outputResult: "<<(cc.outputResult.value()?"true":"false");

			if(cc.outputError)
			os<<",\n\t"<<"outputError: "<<(cc.outputError.value()?"true":"false");

			os<<"\n}";
			return os;
	}
}
