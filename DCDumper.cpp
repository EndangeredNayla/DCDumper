#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include "SectorMap.cpp"
#include "MD5.h"

using namespace std;

int parseargs(int argc, char* argv[]);

//g_special unused, possible todo, auto settings for certain drives
static char g_driveletter;
static int g_special = 0, g_passes = 10, g_interval = 10289, g_cutoff = 549150, g_retries = 3;
static bool g_deltmp = true, g_fakereads = true, g_stepback = true;
static ofstream g_log("dcd.log", ios::app | ios::out);

int main(int argc, char* argv[])
{

	if (parseargs(argc, argv) != 0)
		return 1;


	g_log.write("\n\n---\n", 7);
	g_log.write("DCdumper.exe ", 13);
	for (int i = 0; i < argc; i++)
	{
		printandlogtext(string(argv[i]));
		printandlogtext(" ", 1);
	}
	printandlogtext("\n\n", 2);

	//checks
	HANDLE testhandle;
	if ((testhandle = Section().OpenVolume(g_driveletter)) == INVALID_HANDLE_VALUE)
	{
		printandlogtext("Could not get handle to CD (possible wrong input).\n", 51);
		g_log.close();
		return 1;
	}
	printandlogtext("Handle acquired.\n", 17);

	printandlogtext("Load disc: ", 11);
	DWORD dw;
	if (!DeviceIoControl(testhandle, IOCTL_STORAGE_LOAD_MEDIA, NULL, 0, NULL, 0, &dw, NULL))
	{
		printandlogtext("Failed.\n", 8);
		g_log.close();
		return 1;
	}
	printandlogtext("Done.\n", 6);
	Section().CloseVolume(testhandle);


	//[todo] read toc
	if (false)
	{
		printandlogtext("Error reading TOC\n", 18);
		g_log.close();
		return 1;
	}

	if (false)
	{
		printandlogtext("TOC indicates trap disc not loaded (too many tracks).\n", 54);
		g_log.close();
		return 1;
	}

	//printandlogtext("TOC read.\n",10);
	//[/todo]

	SectorMap map = SectorMap();
	printandlogtext("Sector map created.\n", 20);

	for (int i = 0; i < g_passes; i++)
	{
		if (!map.isdone())
		{
			printandlogtext("\n..................:::::::::::::::: PASS ", 41);
			printandlognumber(i + 1);
			printandlogtext(" ::::::::::::::::..................\n", 36);
			map.pass();
		}
		else
		{
			break;
		}
	}

	if (map.isdone())
	{
		//all dumped successfully, create dense.bin
		if (map.numsections == 1)
		{
			//rename to dense.bin
			rename(map.sections[0].name.c_str(), "dense.bin");
			printandlogtext("Renamed to dense.bin, pass this to ice.exe\n", 43);
		}
		else
		{
			printandlogtext("Creating dense.bin: ", 20);
			ofstream outfile("dense.bin", ios::binary);
			char* buffer = new char[2352];
			for (int i = 0; i < map.numsections; i++)
			{
				ifstream infile(map.sections[i].name, ios::binary);
				for (int j = 0; j < map.sections[i].numsectors; j++)
				{
					infile.read(buffer, 2352);
					outfile.write(buffer, 2352);
				}
				infile.close();
			}
			outfile.close();
			printandlogtext("Done. Pass this to ice.exe\n", 27);
			if (g_deltmp)
			{
				for (int i = 0; i < map.numsections; i++)
				{
					map.sections[i].deletefiles();
				}
			}

			g_log.close();
			return 0;
		}
	}
	else
	{
		for (int i = 0; i < map.numsections; i++)
		{
			if (!map.sections[i].done)
			{
				printandlogtext("Sector read error between sectors ", 34);
				printandlognumber(map.sections[i].initialsector);
				printandlogtext(" and ", 5);
				printandlognumber(map.sections[i].initialsector + map.sections[i].numsectors - 1);
				printandlogtext(" inclusive.\n", 12);
			}
		}
		printandlogtext("Clean the disc and try again.\n---", 33);

		g_log.close();
		return 1;
	}
}

int parseargs(int argc, char* argv[])
{
	string helptest, currop;
	if (argc == 1)
	{
		printf("Not enough arguments. -h for help.\n");
		printf("Usage: DCDump.exe #:#:# drive_letter [options]\n");
		return 1;
	}
	else if (argc >= 2)
	{
		helptest = string(argv[1]);
		if (helptest == string("-h"))
		{
			printf("Author: jamjam\n");
			printf("Version: 0.42a\n");
			printf("Usage: ");
			printf(argv[0]);
			printf(" drive_letter [options]\n\n");
			printf("[options]\n");
			printf("-h      - This help.\n\n");
			printf("-c#     - Cut-off: After which disc is dumped in one lump instead of sections\n");
			printf("          Min: 44990 (all at once)  Max: 549150 (no cut-off)  Default: 549150\n\n");
			printf("-dt -df - Delete temporary files: Delete sections and hashes after creating dense.bin\n");
			printf("          Default true\n\n");
			printf("-ft -ff - Fake reads: Fake read nearby to try and kick program out of read errors\n");
			printf("          Default true\n\n");
			printf("-i#     - Set number of sectors per section\n");
			printf("          Min: 26  Max: 504161 (all at once)  Default: 10289\n\n");
			printf("-p#     - Passes: Set maximum number of passes\n");
			printf("          Min: 2  Max: 100  Default: 10\n\n");
			printf("-st -sf - Step back: Fake reads after a pass to step to beginning\n");
			printf("          Default: true\n\n");
			printf("-t#     - Reread attempts: Max number of re-read attempts on a read error\n");
			printf("          Min: 0  Max: 20  Default: 3\n\n");
			return 1;
		}
		else if (argv[1][0] == '-')
		{
			printf("Core commandline arguments missing (drive letter)\n");
			printf("Usage: DCDump.exe drive_letter [options]\n");
			return 1;
		}
		else if ((argv[1][0] < 'a' || argv[1][0]>'z') && (argv[1][0] < 'A' || argv[1][0]>'Z'))
		{
			printf("Drive letter invalid (must be in range a-z or A-Z)");
			return 1;
		}
		else
		{
			g_driveletter = argv[1][0];
			for (int i = 2; i < argc; i++)
			{
				//for every option
				currop = string(argv[i]);
				if (*(argv[i]) != '-' || currop.length() < 2)
				{
					printf("\"");
					printf(argv[i]);
					printf("\" is not a valid option...skipping.\n");
				}
				else if (currop.at(1) == 'c' || currop.at(1) == 'C')
				{
					//cut-off point
					if (currop.length() < 3)
					{
						printf("\"");
						printf(argv[i]);
						printf("\" is not a valid option...skipping.\n");
					}
					else
					{
						int cutoff = atoi(currop.substr(2, currop.length() - 2).c_str());
						if (cutoff > 44989 && cutoff < 549151)
						{
							g_cutoff = cutoff;
						}
						else
						{
							printf("\"");
							printf(argv[i]);
							printf("\" is not a valid option...skipping.\n");
						}
					}
				}
				else if (currop.at(1) == 'd' || currop.at(1) == 'D')
				{
					//delete tmp files?
					if (currop.length() < 3)
					{
						printf("\"");
						printf(argv[i]);
						printf("\" is not a valid option...skipping.\n");
					}
					else
					{
						if (currop.at(2) == 'f' || currop.at(2) == 'F')
						{
							g_deltmp = false;
						}
						else if (currop.at(2) == 't' || currop.at(2) == 'T')
						{
							g_deltmp = true;
						}
						else
						{
							printf("\"");
							printf(argv[i]);
							printf("\" is not a valid option...skipping.\n");
						}
					}
				}
				else if (currop.at(1) == 'f' || currop.at(1) == 'F')
				{
					//try fake reads when read error?
					if (currop.length() < 3)
					{
						printf("\"");
						printf(argv[i]);
						printf("\" is not a valid option...skipping.\n");
					}
					else
					{
						if (currop.at(2) == 'f' || currop.at(2) == 'F')
						{
							g_fakereads = false;
						}
						else if (currop.at(2) == 't' || currop.at(2) == 'T')
						{
							g_fakereads = true;
						}
						else
						{
							printf("\"");
							printf(argv[i]);
							printf("\" is not a valid option...skipping.\n");
						}
					}
				}
				else if (currop.at(1) == 'i' || currop.at(1) == 'I')
				{
					//interval
					if (currop.length() < 3)
					{
						printf("\"");
						printf(argv[i]);
						printf("\" is not a valid option...skipping.\n");
					}
					else
					{
						int interval = atoi(currop.substr(2, currop.length() - 2).c_str());
						if (interval > 25 && interval < 504162)
						{
							g_interval = interval;
						}
						else
						{
							printf("\"");
							printf(argv[i]);
							printf("\" is not a valid option...skipping.\n");
						}
					}
				}
				else if (currop.at(1) == 'p' || currop.at(1) == 'P')
				{
					//number of passes
					if (currop.length() < 3)
					{
						printf("\"");
						printf(argv[i]);
						printf("\" is not a valid option...skipping.\n");
					}
					else
					{
						int passes = atoi(currop.substr(2, currop.length() - 2).c_str());
						if (passes > 1 && passes < 101)
						{
							g_passes = passes;
						}
						else
						{
							printf("\"");
							printf(argv[i]);
							printf("\" is not a valid option...skipping.\n");
						}
					}
				}
				else if (currop.at(1) == 's' || currop.at(1) == 'S')
				{
					//try fake reads when read error?
					if (currop.length() < 3)
					{
						printf("\"");
						printf(argv[i]);
						printf("\" is not a valid option...skipping.\n");
					}
					else
					{
						if (currop.at(2) == 'f' || currop.at(2) == 'F')
						{
							g_stepback = false;
						}
						else if (currop.at(2) == 't' || currop.at(2) == 'T')
						{
							g_stepback = true;
						}
						else
						{
							printf("\"");
							printf(argv[i]);
							printf("\" is not a valid option...skipping.\n");
						}
					}
				}
				else if (currop.at(1) == 't' || currop.at(1) == 'T')
				{
					//number of retries per read attempt
					if (currop.length() < 3)
					{
						printf("\"");
						printf(argv[i]);
						printf("\" is not a valid option...skipping.\n");
					}
					else
					{
						int retries = atoi(currop.substr(2, currop.length() - 2).c_str());
						if (retries >= 0 && retries < 21)
						{
							g_retries = retries;
						}
						else
						{
							printf("\"");
							printf(argv[i]);
							printf("\" is not a valid option...skipping.\n");
						}
					}
				}
				else
				{
					printf("\"");
					printf(argv[i]);
					printf("\" is not a valid option...skipping.\n");
				}
			}
			return 0;
		}
	}
	return 1;
}