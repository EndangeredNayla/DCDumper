#ifndef SECTION
#define SECTION

#include "MD5.h"
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>

using namespace std;

extern char g_driveletter;
extern int g_special, g_passes, g_interval, g_cutoff, g_retries;
extern bool g_deltmp, g_fakereads, g_stepback;
extern ofstream g_log;

//cdread v1.01 truman verbatim
typedef struct _SCSI_PASS_THROUGH_DIRECT {
	USHORT Length;
	UCHAR ScsiStatus;
	UCHAR PathId;
	UCHAR TargetId;
	UCHAR Lun;
	UCHAR CdbLength;
	UCHAR SenseInfoLength;
	UCHAR DataIn;
	ULONG DataTransferLength;
	ULONG TimeOutValue;
	PVOID DataBuffer;
	ULONG SenseInfoOffset;
	UCHAR Cdb[16];
}SCSI_PASS_THROUGH_DIRECT, *PSCSI_PASS_THROUGH_DIRECT;

static void printandlogtext(string str);
static void printandlogtext(const char*str,int num);
static void lognumber(int num);
static void printandlognumber(int num);

static void printandlogtext(string str)
{
  g_log.write(str.c_str(),str.length());
  printf(str.c_str());
}
static void printandlogtext(const char*str,int num)
{
  g_log.write(str,num);
  printf(str);
}
static void lognumber(int num)
{
  char buffer[10];
  sprintf(buffer,"%d",num);
  g_log.write(string(buffer).c_str(),string(buffer).length());
}
static void printandlognumber(int num)
{
  lognumber(num);
  printf("%d",num);
}

class Section
{
  public:
  //cdread v1.01 truman verbatim
  static HANDLE OpenVolume(char cDriveLetter)
  {
    HANDLE hVolume;
    UINT uDriveType;
    char szVolumeName[8];
    char szRootName[5];
    DWORD dwAccessFlags;
    
    szRootName[0]=cDriveLetter;
    szRootName[1]=':';
    szRootName[2]='\\';
    szRootName[3]='\0';

    uDriveType = GetDriveType(L"szRootName");

    switch(uDriveType) {
    case DRIVE_CDROM:        
        dwAccessFlags = GENERIC_READ | GENERIC_WRITE;
        break;
    default:
        printf("Drive type is not CDROM/DVD.\n");
        return INVALID_HANDLE_VALUE;
    }
    
    szVolumeName[0]='\\';
    szVolumeName[1]='\\';
    szVolumeName[2]='.';
    szVolumeName[3]='\\';
    szVolumeName[4]=cDriveLetter;
    szVolumeName[5]=':';
    szVolumeName[6]='\0';

    hVolume = CreateFile(   L"szVolumeName",
                            dwAccessFlags,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );

    if (hVolume == INVALID_HANDLE_VALUE)
        printf("Could not create handle for CDROM/DVD.\n");
    
    return hVolume;
  }

  //cdread v1.01 truman verbatim
  BOOL CloseVolume(HANDLE hVolume)
  {
    return CloseHandle(hVolume);
  }

  //cdread v1.01 truman modified
  #define SEC_AT_ONCE 26 //number of sectors to read with one DeviceIO call, max 255
  BOOL ReadCD(string &hashret, char cDriveLetter, long int MMC_LBA_sector, long int numsec, string name, int trycount, bool stepsuppress=false)
  {

    BOOL success;
    HANDLE hVolume;
    hVolume = OpenVolume(cDriveLetter);
    long int MMC_LBA_sector2;
	//long int numsec2;
	
	byte DataBuf[2352*SEC_AT_ONCE]; //buffer for holding sector data from drive
	SCSI_PASS_THROUGH_DIRECT sptd;
    if (hVolume != INVALID_HANDLE_VALUE)
    {
        sptd.Length=sizeof(sptd);
        sptd.PathId=0;//auto
        sptd.TargetId=0;//auto
        sptd.Lun=0;//auto
        sptd.CdbLength=12; //CDB size is 12 for ReadCD MMC1 command
        sptd.SenseInfoLength=0; //Don't return any sense data
        sptd.DataIn=1; //There will be data from drive
        sptd.TimeOutValue=5;  //SCSI timeout value (5 seconds - maybe it should be longer)
        sptd.DataBuffer=(PVOID)&(DataBuf);
        sptd.SenseInfoOffset=0;

        //CDB with values for ReadCD CDB12 command.  The values were taken from MMC1 draft paper.
        sptd.Cdb[0]=0xBE;  //Code for ReadCD12 command
        sptd.Cdb[1]=0;        
        
               
        
		sptd.Cdb[6]=0;  //No. of sectors to read from CD byte 2 (MSB)
        sptd.Cdb[7]=0;  //No. of sectors to read from CD byte 1


        sptd.Cdb[9]=0xF8;  //Raw read, 2352 bytes per sector
        sptd.Cdb[10]=0;  //Sub-channel selection bits.
        sptd.Cdb[11]=0;
        sptd.Cdb[12]=0;
        sptd.Cdb[13]=0;
        sptd.Cdb[14]=0;
        sptd.Cdb[15]=0;

		sptd.Cdb[5]=byte(MMC_LBA_sector);   //Least sig byte of LBA sector no. to read from CD
        MMC_LBA_sector2=MMC_LBA_sector>>8;
        sptd.Cdb[4]=byte(MMC_LBA_sector2);  //2nd byte of:
        MMC_LBA_sector2=MMC_LBA_sector2>>8;
        sptd.Cdb[3]=byte(MMC_LBA_sector2);  //3rd byte of:
        MMC_LBA_sector2=MMC_LBA_sector2>>8;
        sptd.Cdb[2]=byte(MMC_LBA_sector2);  //Most significant byte 

        DWORD dwBytesReturned;
    
		MD5 md5;
		FILE *fp;
	    fp = fopen(name.c_str(), "w+b" );

		sptd.DataTransferLength=2352*SEC_AT_ONCE; //Size of data
		sptd.Cdb[8]=SEC_AT_ONCE;  //No. of sectors to read from CD byte 0 (LSB)
        for(int i=0;i<numsec/SEC_AT_ONCE;i++)
		{//get most
		  


		  success=DeviceIoControl(hVolume,
                                0x4D014,               
                                (PVOID)&sptd, (DWORD)sizeof(SCSI_PASS_THROUGH_DIRECT),
                                NULL, 0,                                                        
                                &dwBytesReturned,
                                NULL);
		  if(success)
          {            
		    md5.update(DataBuf, SEC_AT_ONCE*2352);
	        fwrite( DataBuf, 1, SEC_AT_ONCE*2352, fp );
          }
          else
          {
            break;
          }
		  //update start sector every time
		  MMC_LBA_sector+=SEC_AT_ONCE;
		  sptd.Cdb[5]=byte(MMC_LBA_sector);   //Least sig byte of LBA sector no. to read from CD
          MMC_LBA_sector2=MMC_LBA_sector>>8;
          sptd.Cdb[4]=byte(MMC_LBA_sector2);  //2nd byte of:
          MMC_LBA_sector2=MMC_LBA_sector2>>8;
          sptd.Cdb[3]=byte(MMC_LBA_sector2);  //3rd byte of:
          MMC_LBA_sector2=MMC_LBA_sector2>>8;
          sptd.Cdb[2]=byte(MMC_LBA_sector2);  //Most significant byte 
		}
        if(numsec%SEC_AT_ONCE!=0&&success)
		{//get remaining
		  sptd.DataTransferLength=2352*(numsec%SEC_AT_ONCE);
		  sptd.Cdb[8]=numsec%SEC_AT_ONCE;  //No. of sectors to read from CD byte 0 (LSB)
		  success=DeviceIoControl(hVolume,
                                0x4D014,               
                                (PVOID)&sptd, (DWORD)sizeof(SCSI_PASS_THROUGH_DIRECT),
                                NULL, 0,                                                        
                                &dwBytesReturned,
                                NULL);
		  if(success)
          {            
		    md5.update(DataBuf, (numsec%SEC_AT_ONCE)*2352);
	        fwrite( DataBuf, 1, (numsec%SEC_AT_ONCE)*2352, fp );
          }
		}

		if(!success||stepsuppress)
		{
		  fclose(fp);
		  remove(name.c_str());
		  if(!stepsuppress)
		  {
			printandlogtext(" - read error.\n",15);
		  }
		  CloseVolume(hVolume);
		  if(trycount<g_retries)
		  {
			if(g_fakereads)
			{
			  printandlogtext("Fake read. ",11);
			  string fh;
			  if(MMC_LBA_sector<250000)
				ReadCD(fh, cDriveLetter, MMC_LBA_sector+20000, 26, "fake.read", g_retries,true);
			  else
				ReadCD(fh, cDriveLetter, MMC_LBA_sector-20000, 26, "fake.read", g_retries,true);
			  remove("fake.read");
			}
			printandlogtext("Retry",5);
		    return ReadCD(hashret, cDriveLetter, MMC_LBA_sector, numsec, name, trycount+1);
		  }
		  return false;
		}

		md5.finalize();
		hashret=md5.hexdigest();
        fclose(fp);
        CloseVolume(hVolume);
        return success;        
    }
    else
    {
        return FALSE;
    }    
  }
//
  bool done;
  int initialsector;
  int numsectors;
  string name;
  string prevhashes;
  vector<string> hashes;

  Section(){}

  Section(int initsec, int numsec)
  {
    initialsector=initsec;
	numsectors=numsec;
    done=false;

	//grab name
	char namebuffer[18];
	sprintf(namebuffer,"%06d-%06d.bin",initsec,initsec+numsec-1);
	name=string(namebuffer);
	prevhashes=name.substr(0,13)+".hsh";

	ifstream ifsbin(name, ifstream::in|ifstream::ate);
	int binsize=ifsbin.tellg();
    if (ifsbin&&binsize==2352*numsectors)
	{
	  //file already exists. Assume program is finishing a previous dump
      done=true;
    }
	else
	{
	  ifstream ihashes(prevhashes, ifstream::in|ifstream::ate);
	  if(ihashes)
	  {
		//previous hashes exist, read them in
		char hashbuf[33];
		hashbuf[32]='\0';
		int hashflength=(int)ihashes.tellg();
		ihashes.seekg(0,ifstream::beg);
		for(int i=0;i<hashflength;i+=32)
		{
		  ihashes.read(hashbuf,32);
		  hashes.push_back(string(hashbuf,32));
		}
	  }
	  ihashes.close();
	}
	ifsbin.close();
  }

  //use direct with rereadmod=g_retries, stepsuppress true, for step back
  void read(int rereadmod=0, bool stepsuppress=false)
  {
	string str;
    if(ReadCD(str,g_driveletter,initialsector,numsectors,name,rereadmod, stepsuppress))
	{
	  //search for match
	  for(unsigned int i=0;i<hashes.size();++i)
	  {
	    if(hashes.at(i)==str)
		{
		  //match found
		  done=true;
		  if(!stepsuppress)
		  {
		    printandlogtext(" - MATCH: ",10);
		    printandlogtext(str);
		    printandlogtext("\n",1);
		  }
		  break;
		}
	  }
	  if(!done)
	  {
	    if(hashes.size()==0)
	    {
		  if(!stepsuppress)
		    printandlogtext(" - Initial dump.\n",17);
	    }
	    else
	    {
		  if(!stepsuppress)
		    printandlogtext(" - Non-Match.\n",14);
	    }
	    hashes.push_back(str);
	    ofstream ofshash(prevhashes,ios::out);
		for(int i=0;i<hashes.size();i++)
	      ofshash.write(hashes[i].c_str(),32);
	    ofshash.close();
	    remove(name.c_str());
	  }
	}
  }


  //use when dense.bin created, cleanup
  void deletefiles()
  {
	remove(name.c_str());
	remove(prevhashes.c_str());
  }

};

#endif