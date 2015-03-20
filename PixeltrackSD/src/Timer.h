/*
Copyright (C) 2013 Stefan Duffner, LIRIS, INSA de Lyon, France

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TL_TIMER_H
#define TL_TIMER_H


namespace TLUtil {

class Timer 
{
	long long mStartTime;

public:
	/// hours
	int hours;
	/// minutes
	int minutes;
	/// seconds
	int seconds;
	/// milliseconds
	int mseconds;
	/// microseconds
	int useconds;
	/// nanoseconds
	int nseconds;
	/// create the timer
	Timer();

	~Timer();

	/** reset the timer 
	
	    The timer will count time starting from now, and the accumulated time is erased.
	*/
	void reset();

	/// stop the timer and return the total accumulated time
	long long stop();

	/// return the total accumulated time
	long long getRunTime();

	/// compute the time difference
	long long computeDelta(long long current_time_, long long previous_time_);
};

}

#endif
