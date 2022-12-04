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
		std::optional<unsigned long long int>  outputMaxLength;
		std::optional<unsigned long long int>  errorMaxLength;
		
	};


	struct PublisherConfig
	{	
		std::optional<unsigned long long int>  inputMaxLength;		
		
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

			if(cc.errorMaxLength)
			os<<",\n\t"<<"errorMaxLength: "<<*cc.errorMaxLength;

			if(cc.outputMaxLength)
			os<<",\n\t"<<"outputMaxLength: "<<*cc.outputMaxLength;

			os<<"\n}";
			return os;
	}

		std::ostream &operator<<(std::ostream &os, const PublisherConfig &cc){
			os<<"{\n";
			if(cc.inputMaxLength)
			os<<",\n\t"<<"inputMaxLength: "<<*cc.inputMaxLength;
			os<<"\n}";
			return os;
	}
}
