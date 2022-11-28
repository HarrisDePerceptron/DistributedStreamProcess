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

class TaskPublisher
{
private:
	
public:
	TaskPublisher() = delete;
	TaskPublisher(TaskPublisher &&) = delete;
	TaskPublisher &operator=(const TaskPublisher &) = delete;
	TaskPublisher &operator=(TaskPublisher &&) = delete;

	Task & task;
	TaskPublisher(Task & task): task{task}
	{

	}

	void publish(const Attrs & values){
		publish(values, "*");
	}
	void publish(const Attrs &values,  const std::string &streamId){

		auto inputStreamName = task.getInputStreamName();
		task.sendMessage(inputStreamName, values, streamId);

	}

};

// inputStreamName => last dependent task + _output
// groupName => Task Name
// Consumer Name => unique arbritarary consumer name ?  taskname + _consumer
// outputStreamName => taskname + _output
// errorStreamName
