#include "time.h"

using namespace std;

CTime::CTime() : hour(0), minute(0), second(0), lastHour(0), lastMinute(0), lastSecond(0), timeSet(false)
{}

int CTime::getHour() const
{
    return hour;
}

int CTime::getMinute() const
{
    return minute;
}

int CTime::getSecond() const
{
    return second;
}

bool CTime::hourChanged()
{
	if(hour != lastHour)
		return true;
	else
		return false;
}

bool CTime::minuteChanged()
{
	if(minute != lastMinute)
		return true;
	else
		return false;
}

bool CTime::secondChanged()
{
	if(second != lastSecond)
		return true;
	else
		return false;
}

void CTime::setHour(int hourArg)
{
  lastHour = hour;
  hour = hourArg;
}

void CTime::setMinute(int minuteArg)
{
  lastMinute = minute;
  minute = minuteArg;
}

void CTime::setSecond(int secondArg)
{
  lastSecond = second;
  second = secondArg;
}

bool CTime::isTimeSet()
{
	if(hour != 0 && minute != 0 && second != 0)
		return true;
	else
		return false;
}
