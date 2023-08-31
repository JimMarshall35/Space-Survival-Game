#include "ScopeTimer.h"

#include <iostream>

std::vector<ScopeTimer_MessageType> ScopeTimer::MessageList = std::vector<ScopeTimer_MessageType>();
ScopeTimer_MessageType ScopeTimer::LastMessage = std::make_pair(0.f, std::string());
unsigned int ScopeTimer::ActiveTimers = 0;

ScopeTimer::ScopeTimer(const char* instanceName)
	: InstanceName(instanceName)
{
	++ActiveTimers;
	Start = Time::now();
}

ScopeTimer::~ScopeTimer()
{
	End = Time::now();
	--ActiveTimers;
	Duration difference = End - Start;
	Millisecond ms = std::chrono::duration_cast<Millisecond>(difference);

	std::string message(InstanceName + " took " + std::to_string(ms.count()) + "ms.");

	LastMessage = std::make_pair(Time::now().time_since_epoch().count(), message);
	MessageList.push_back(LastMessage);

	std::cout << message << std::endl;
}

void ScopeTimer::PrintAllMessages()
{
	std::cout << "DebugTimer - Start Message Log" << std::endl;
	for each(const ScopeTimer_MessageType& message in MessageList)
	{
		std::cout << "[" << message.first << "] " << message.second << std::endl;
	}
	std::cout << "DebugTimer - Start Message Log" << std::endl;
}

void ScopeTimer::ClearMessages(const char* instanceName)
{
	if (ActiveTimers > 0)
	{
		std::cout << instanceName << " - DebugTimer::ClearMessages failed, active timers remaining: " << ActiveTimers << std::endl;
		return;
	}

	LastMessage = std::make_pair(0.f, std::string());
	MessageList.clear();

	std::cout << instanceName << " - DebugTimer::ClearMessages succeeded" << std::endl;
}

// TODO: Output a formatted time message (HH:MM:SS:MS) for better display in PrintAllMessages
const std::string ScopeTimer::GetFormattedTime()
{
	Duration now = Time::now().time_since_epoch();
	auto h = std::chrono::duration_cast<Hour>(now);
	auto m = std::chrono::duration_cast<Minute>(now);
	auto s = std::chrono::duration_cast<Second>(now);
	auto ms = std::chrono::duration_cast<Millisecond>(now);

	return std::string();
}