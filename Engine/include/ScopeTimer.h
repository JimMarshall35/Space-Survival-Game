#pragma once

#include<chrono>
#include <string>
#include <vector>

#define DEBUG_SCOPETIMER ScopeTimer dt(__FUNCTION__);
#define DEBUG_PRINTALL ScopeTimer::PrintAllMessages();
#define DEBUG_CLEARMESSAGES ScopeTimer::ClearMessages(__FUNCTION__);

typedef std::pair<float, std::string> ScopeTimer_MessageType;

class ScopeTimer
{
public:
	ScopeTimer(const char* instanceName);
	~ScopeTimer();

	const std::vector<ScopeTimer_MessageType>& GetDebugMessages() const { return MessageList; }
	const ScopeTimer_MessageType& GetLastMessage() const { return LastMessage; }

	static unsigned int GetActiveTimerCount() { return ActiveTimers; }
	static void PrintAllMessages();
	static void ClearMessages(const char* instanceName);

private:
	const std::string GetFormattedTime();

	typedef std::chrono::high_resolution_clock Time;
	typedef std::chrono::hours Hour;
	typedef std::chrono::minutes Minute;
	typedef std::chrono::seconds Second;
	typedef std::chrono::milliseconds Millisecond;
	typedef std::chrono::duration<float> Duration;
	typedef std::chrono::steady_clock::time_point Timepoint;

	std::string InstanceName;
	Timepoint Start;
	Timepoint End;

	static std::vector<ScopeTimer_MessageType> MessageList;
	static ScopeTimer_MessageType LastMessage;
	static unsigned int ActiveTimers;
};
