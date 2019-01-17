#include <base/utils/misc.h>

#include <cstdlib>

#include <iostream>
#include <ctime>


unsigned int rnd(void)
{
	return std::rand();
}

// This functions really calculates the probability of something...
// It should be used in the enemy AI of any game where needed
// In many cases it is using workarounds, if you see something like that, please change it!
// Due the game resolutions, we specify this per thousand, instead per percent.
// So if you want 80 % you specify 800 here!
// Don't forget that this odd is checked LPS times. Mostly 10% (100) is already a good value to take
bool getProbability(unsigned short prob)
{
	return ( (rnd()%1000) <= prob ) ? true : false;
}



std::string getTimeStr()
{
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[80];

  time (&rawtime);
  timeinfo = localtime(&rawtime);

  strftime(buffer,80,"%d-%m-%Y %H:%M:%S",timeinfo);
  std::string str(buffer);

  return str;
}
