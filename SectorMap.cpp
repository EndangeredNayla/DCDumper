#ifndef SECTOR_MAP
#define SECTOR_MAP

#include "Section.cpp"

extern char g_driveletter;
extern int g_special, g_passes, g_interval, g_cutoff, g_retries;
extern bool g_deltmp, g_fakereads, g_stepback;
extern ofstream g_log;

class SectorMap
{
public:
  Section* sections;
  int numsections;

  SectorMap()
  {
	if(g_cutoff==44990)
	{
	  sections=new Section[1];
	  numsections=1;
	  sections[0]=Section(44990,504161);
	}
	else
	{
	  int startsector=44990;
	  int ceiling=g_cutoff-44990+1;
	  int numwhole=ceiling/g_interval;
	  bool partial=ceiling%g_interval!=0||g_cutoff!=549150;
	  sections=new Section[numwhole+partial];
	  numsections=numwhole+partial;
	  for(int i=0;i<numwhole;i++)
	  {
	    sections[i]=Section(startsector,g_interval);
	    startsector+=g_interval;
	  }
	  if(partial)
	  {
	    sections[numsections-1]=Section(startsector,549151-startsector);
	  }
	}
  }

  void pass()
  {
	if(!isdone())
	{
	  for(int i=0;i<numsections;i++)
	  {
		if(!sections[i].done)
		{
	      printandlogtext("Reading section ",16);
		  printandlognumber(i+1);
		  printandlogtext(": ",2);
		  printandlogtext(sections[i].name.substr(0,13));
		  sections[i].read();
	    }
	  }
	  if(g_stepback)
	  {
		Section fake;
		for(int i=549000;i>44990;i-=20000)
		{
		  fake=Section(i,26);
		  fake.read(g_retries,true);
		  fake.deletefiles();
		}
	  }
	}
  }

  bool isdone()
  {
	for(int i=0;i<numsections;i++)
	{
	  if(!sections[i].done)
	  {
	    return false;
	  }
	}
	return true;
  }

};

#endif
