#pragma once

#include <iostream>
#include <vector>
#include <thread>
#include <sw/redis++/redis++.h>
#include "utilities.h"

#include <fmt/core.h>

#include "task_response.h"


#include <functional>

#include "task.h"
#include "config.h"

class TaskPublisher
{
private:
	
	unsigned long long int inputMaxLength{0};
public:
	TaskPublisher() = delete;
	TaskPublisher(TaskPublisher &&) = delete;
	TaskPublisher &operator=(const TaskPublisher &) = delete;
	TaskPublisher &operator=(TaskPublisher &&) = delete;

	Task & task;

	TaskPublisher(Task & _task): task{_task}
	{	

	}

	TaskPublisher(Task & _task, const DistributedTask::PublisherConfig & config):TaskPublisher(_task)
	{
		if(config.inputMaxLength){
			inputMaxLength = *config.inputMaxLength;
		}
	}

	std::string publish(const Attrs & values){
		return publish(values, "*");
	}
	std::string publish(const Attrs &values,  const std::string &streamId) const{

		auto inputStreamName = task.getInputStreamName();
		return task.sendMessage(inputStreamName, values, streamId,inputMaxLength);

	}

};

// inputStreamName => last dependent task + _output
// groupName => Task Name
// Consumer Name => unique arbritarary consumer name ?  taskname + _consumer
// outputStreamName => taskname + _output
// errorStreamName
