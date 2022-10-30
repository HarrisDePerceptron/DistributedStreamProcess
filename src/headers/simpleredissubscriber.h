#pragma once;

#include <iostream>
#include <vector>
#include <thread>

#include <sw/redis++/redis++.h>

namespace RedisNS = sw::redis;

namespace SimpleRedis
{
	class SimpleRedisSubscriber
	{

	private:
		RedisNS::Subscriber subscriber;
		std::thread t;

		std::atomic<bool> started{false};

		std::string name;
		std::string channelName;

		std::vector<std::function<void(std::string, std::string)>> callbacks;

		void loop()
		{

			std::cout << "starting subscriber: " << name << std::endl;
			while (started)
			{

				try
				{

					subscriber.consume();
				}
				catch (const RedisNS::TimeoutError &e)
				{
					continue;
				}
				catch (const RedisNS::Error &err)
				{
					std::cout << "pub/sub error" << err.what() << std::endl;
				}
			}
			std::cout << "stopping subscriber: " << name << std::endl;
		}

		std::thread init()
		{
			std::thread t{[&]()
						  {
							  this->loop();
						  }};
			return t;
		}

		void prepareSubscriber()
		{

			subscriber.on_message([&](std::string channel, std::string msg)
								  {
								  for (auto &c : callbacks)
								  {
									  c(channel, msg);
								  } });

			subscriber.on_pmessage([](std::string pattern, std::string channel, std::string msg)
								   { std::cout << "pattern(" << pattern << ") "
											   << "channel(" << channel << "): " << msg << std::endl; });

			subscriber.on_meta([](RedisNS::Subscriber::MsgType type, RedisNS::OptionalString channel, long long num)
							   {
			std::string ch = channel.value();
            std::cout<<"channel("<<ch<<"): "<<num<<std::endl; });
		}

	public:
		SimpleRedisSubscriber(const SimpleRedisSubscriber &) = delete;
		SimpleRedisSubscriber(SimpleRedisSubscriber &&) = delete;
		SimpleRedisSubscriber() = delete;
		SimpleRedisSubscriber &operator=(const SimpleRedisSubscriber &) = delete;

		SimpleRedisSubscriber(RedisNS::Redis &redis, std::string channel, std::string subscriberName) : subscriber(redis.subscriber()), name{subscriberName}, channelName{channel}
		{

			prepareSubscriber();
		}

		void start()
		{
			if (started)
			{
				return;
			}
			subscriber.subscribe(channelName);
			started = true;
			t = init();
		}

		void stop()
		{
			if (!started)
			{
				return;
			}
			started = false;
			subscriber.unsubscribe();
			t.join();
		}

		void addCallback(std::function<void(std::string, std::string)> callback)
		{
			callbacks.push_back(callback);
		}

		virtual ~SimpleRedisSubscriber()
		{

			stop();
			std::cout << "Subscriber: " << name << " destroyed" << std::endl;
		}
	};


}
