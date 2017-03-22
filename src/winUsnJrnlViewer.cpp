// Copyright 2017 Matthew A. Kucenski
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//#define _DEBUG_ 1

#include "libWinUsnJrnl/src/winUsnJrnlRecordsFile.h"
#include "libtimeUtils/src/timeZoneCalculator.h"
#include "libtimeUtils/src/timeUtils.h"

#include <popt.h>

#include <string>
#include <vector>
using namespace std;

#include "misc/stringType.h"
#include "misc/poptUtils.h"

#define OPT_DELIMITED		10
#define OPT_START_DATE		20
#define OPT_END_DATE			30
#define OPT_WITH_FILENAME	40
#define OPT_MACTIME			50
#define OPT_TIMEZONE			60
#define OPT_VERSION			200

int main(int argc, const char** argv) {
	int rv = EXIT_FAILURE;
	
	bool bDelimited = false;
	string strDateStart;
	string strDateEnd;
	bool bWithFilename = false;
	bool bMactime = false;
	
	timeZoneCalculator tzcalc;

	struct poptOption optionsTable[] = {
		{"delimited", 			'd',	POPT_ARG_NONE,		NULL,	OPT_DELIMITED,			"Display in comma-delimited format.", NULL},
		{"start-date", 		0,		POPT_ARG_STRING,	NULL,	OPT_START_DATE, 		"Only display entries recorded after the specified date.", "mm/dd/yyyy"},
		{"end-date", 		 	0,		POPT_ARG_STRING,	NULL,	OPT_END_DATE, 			"Only display entries recorded before the specified date.", "mm/dd/yyyy"},
		{"with-filename",		'H',	POPT_ARG_NONE,		NULL,	OPT_WITH_FILENAME,	"Display filename in output. Useful when batch processing multiple files.", NULL},
		{"mactime",				'm', 	POPT_ARG_NONE, 	NULL,	OPT_MACTIME, 			"Display in the SleuthKit's mactime format."},
		{"timezone", 			'z',	POPT_ARG_STRING,	NULL,	OPT_TIMEZONE,			"POSIX timezone string (e.g. 'EST-5EDT,M4.1.0,M10.1.0' or 'GMT-5') to be used when displaying data. Defaults to GMT.", "zone"},
		{"version",	 			0,		POPT_ARG_NONE,		NULL,	OPT_VERSION,			"Display version.", NULL},
		POPT_AUTOHELP
		{NULL, 				 	0, 	POPT_ARG_NONE, 	NULL, 0, 						NULL, NULL}
	};
	poptContext optCon = poptGetContext(NULL, argc, argv, optionsTable, 0);
	poptSetOtherOptionHelp(optCon, "[options] <filename> [<filename>] ...");
	
	if (argc < 2) {
		poptPrintUsage(optCon, stderr, 0);
		exit(EXIT_FAILURE);
	}

	string strTmp;
	int iOption = poptGetNextOpt(optCon);
	while (iOption >= 0) {
		switch (iOption) {
			case OPT_DELIMITED:
				bDelimited = true;
				break;
			case OPT_START_DATE:
				strDateStart = poptGetOptArg(optCon);
				if (strDateStart.length() != 10) {
					usage(optCon, "Invalid start date value", "e.g., mm/dd/yyyy");
					exit(EXIT_FAILURE);
				}
				break;
			case OPT_END_DATE:
				strDateEnd = poptGetOptArg(optCon);
				if (strDateEnd.length() != 10) {
					usage(optCon, "Invalid end date value", "e.g., mm/dd/yyyy");
					exit(EXIT_FAILURE);
				}
				break;
			case OPT_WITH_FILENAME:
				bWithFilename = true;
				break;
			case OPT_MACTIME:
				bMactime = true;
				break;
			case OPT_TIMEZONE:
				if (tzcalc.setTimeZone(poptGetOptArg(optCon)) >= 0) {
				} else {
					usage(optCon, "Invalid time zone string", "e.g. 'EST-5EDT,M4.1.0,M10.1.0' or 'GMT-5'");
					exit(EXIT_FAILURE);
				}
				break;
			case OPT_VERSION:
				version(PACKAGE, VERSION);
				exit(EXIT_SUCCESS);
				break;
		}
		iOption = poptGetNextOpt(optCon);
	}
	
	if (iOption != -1) {
		usage(optCon, poptBadOption(optCon, POPT_BADOPTION_NOALIAS), poptStrerror(iOption));
		exit(EXIT_FAILURE);
	}
	
	const char* cstrFilename = poptGetArg(optCon);
	while (cstrFilename) {
		arguments.filenameVector.push_back(cstrFilename);
		cstrFilename = poptGetArg(optCon);
	}
	
	if (arguments.filenameVector.size() < 1) {
		usage(optCon, "You must specify at least one file", "e.g., $Extend\$UsnJrnl:$J");
		exit(EXIT_FAILURE);
	}
	
	if (arguments.bDelimited) {
		cout << "Time Zone: \"" << tzcalc.getTimeZoneString() << "\"" << endl;		//Display the timezone so that the reader knows which zone was used for this output
		if ((arguments.filenameVector.size() > 1 && arguments.bNoFilename == false) || arguments.bWithFilename == true) {
			printf("File,");
		}
		printf("Record,Offset,Type,Date,Time,Source,Category,Event,SID,Computer\n");
	}
	
	long lRecordCount = 0;
	for (vector<string>::iterator it = arguments.filenameVector.begin(); it != arguments.filenameVector.end(); it++) {
		winEventFile eventFile(*it);

		if (arguments.bMactime) {
			winEvent* pEvent = NULL;
			while (eventFile.getNextRecord(&pEvent) == WIN_EVENT_SUCCESS) {
				if (displayEvent(pEvent, &arguments) == true) {
					lRecordCount++;
					
					string strTimeWritten = getTimeString(tzcalc.calculateLocalTime(getFromUnix32(pEvent->getTimeWritten())));
					string strDateWritten = getDateString(tzcalc.calculateLocalTime(getFromUnix32(pEvent->getTimeWritten())));
					string strTimeGenerated = getTimeString(tzcalc.calculateLocalTime(getFromUnix32(pEvent->getTimeGenerated())));
					string strDateGenerated = getDateString(tzcalc.calculateLocalTime(getFromUnix32(pEvent->getTimeGenerated())));
			
					printf("USN|%s|%lu|%lu||%ls||%s|%ls||%s||%lu|%lu||",
							it->c_str(),
							pEvent->getRecordNumber(),
							pEvent->getEventCode(),
							pEvent->getSourceName().c_str(),
							//pEvent->getSIDString().c_str(),
							(matchSIDtoUsernames ? pwdFile.getUsernameBySID(pEvent->getSIDString()).c_str() : pEvent->getSIDString().c_str()),
							pEvent->getComputerName().c_str(),
							getEventTypeString(pEvent->getEventType()).c_str(),
							pEvent->getTimeWritten(),
							pEvent->getTimeGenerated()
					);
					printf("\n");
				}	//if (displayEvent(pEvent, &arguments) == true) {
				delete pEvent;
				pEvent = NULL;
			}	//while (eventFile.getNextRecord(&pEvent) == WIN_EVENT_SUCCESS) {
		} else {	
			if (arguments.bDelimited) {
				winEvent* pEvent = NULL;
				while (eventFile.getNextRecord(&pEvent) == WIN_EVENT_SUCCESS) {
					if (displayEvent(pEvent, &arguments) == true) {
						lRecordCount++;

						string strTimeWritten = getTimeString(tzcalc.calculateLocalTime(getFromUnix32(pEvent->getTimeWritten())));
						string strDateWritten = getDateString(tzcalc.calculateLocalTime(getFromUnix32(pEvent->getTimeWritten())));
						string strTimeGenerated = getTimeString(tzcalc.calculateLocalTime(getFromUnix32(pEvent->getTimeGenerated())));
						string strDateGenerated = getDateString(tzcalc.calculateLocalTime(getFromUnix32(pEvent->getTimeGenerated())));

						if ((arguments.filenameVector.size() > 1 && arguments.bNoFilename == false) || arguments.bWithFilename == true) {
							printf("%s,", it->c_str());
						}
						printf("%lu,0x%lx,%s,%s,%s,%ls,%d,%lu,%s,%ls",
								pEvent->getRecordNumber(),
								pEvent->getRecordOffset(),
								getEventTypeString(pEvent->getEventType()).c_str(),
								strDateGenerated.c_str(),//pEvent->getDateGeneratedString().c_str(),
								strTimeGenerated.c_str(),//pEvent->getTimeGeneratedString().c_str(),
								pEvent->getSourceName().c_str(),
								pEvent->getEventCategory(),
								pEvent->getEventCode(),
								//pEvent->getSIDString().c_str(),
								(matchSIDtoUsernames ? pwdFile.getUsernameBySID(pEvent->getSIDString()).c_str() : pEvent->getSIDString().c_str()),
								pEvent->getComputerName().c_str()
						);
						if (arguments.bStringColumns == true) {
							vector<string_t> vStrings;
							if (pEvent->getStrings(&vStrings) == WIN_EVENT_SUCCESS) {
								for (vector<string_t>::iterator it = vStrings.begin(); it != vStrings.end(); it++) {
									printf(",\"%ls\"", removeNewLines(&(*it), STR(" _ ")).c_str());
								}
							}	//if (pEvent->getStrings(&vStrings) == WIN_EVENT_SUCCESS) {
						}	//if (arguments.bStringColumns == true) {
						printf("\n");
					}	//if (displayEvent(pEvent->getEventCode(), &arguments) == true) {
					
					delete pEvent;
					pEvent = NULL;
				}	//while (eventFile.getNextRecord(&pEvent) == WIN_EVENT_SUCCESS) {
			} else {	//if (bDelimited) {
				winEvent* pEvent = NULL;
				while (eventFile.getNextRecord(&pEvent) == WIN_EVENT_SUCCESS) {
					if (displayEvent(pEvent, &arguments) == true) {
						lRecordCount++;

						string strTimeWritten = getTimeString(tzcalc.calculateLocalTime(getFromUnix32(pEvent->getTimeWritten())));
						string strDateWritten = getDateString(tzcalc.calculateLocalTime(getFromUnix32(pEvent->getTimeWritten())));
						string strTimeGenerated = getTimeString(tzcalc.calculateLocalTime(getFromUnix32(pEvent->getTimeGenerated())));
						string strDateGenerated = getDateString(tzcalc.calculateLocalTime(getFromUnix32(pEvent->getTimeGenerated())));

						if ((arguments.filenameVector.size() > 1 && arguments.bNoFilename == false) || arguments.bWithFilename == true) {
							printf("%s ", it->c_str());
						}
						printf("%lu: (offset=0x%lx, length=0x%lx)\n\tDate Generated:\t\t%s\t\tSource:\t\t%ls\n\tTime Generated (GMT):\t%s\t\tCategory:\t%u\n\tDate Written:\t\t%s\t\tEvent ID:\t%lu\n\tTime Written (GMT):\t%s\t\tUser:\t\t%s\n\tType:\t\t\t%-10s\t\tComputer:\t%ls\n",
								pEvent->getRecordNumber(),
								pEvent->getRecordOffset(),
								pEvent->getRecordLength(),
								strDateGenerated.c_str(),//pEvent->getDateGeneratedString().c_str(),
								pEvent->getSourceName().c_str(),
								strTimeGenerated.c_str(),//pEvent->getTimeGeneratedString().c_str(),
								pEvent->getEventCategory(),
								strDateWritten.c_str(),//pEvent->getDateWrittenString().c_str(),
								pEvent->getEventCode(),
								strTimeWritten.c_str(),//pEvent->getTimeWrittenString().c_str(),
								//pEvent->getSIDString().c_str(),
								(matchSIDtoUsernames ? pwdFile.getUsernameBySID(pEvent->getSIDString()).c_str() : pEvent->getSIDString().c_str()),
								getEventTypeString(pEvent->getEventType()).c_str(),
								pEvent->getComputerName().c_str()
						);
			
						vector<string_t> vStrings;
						if (pEvent->getStrings(&vStrings) == WIN_EVENT_SUCCESS) {
							printf("\n\tStrings: (offset=0x%lx, count=%u)\n", pEvent->getStringsOffset(), pEvent->getNumStrings());
					
							int i=1;
							for (vector<string_t>::iterator it = vStrings.begin(); it != vStrings.end(); it++) {
								printf("\t%7d:\t%ls\n", i++, removeNewLines(&(*it), STR(" _ ")).c_str());
							}
						}	//if (pEvent->getStrings(&vStrings) == WIN_EVENT_SUCCESS) {
						
						if (pEvent->getDataLength() > 0) {
							unsigned char* pData = NULL;
							if (pEvent->getData((char**)&pData) == WIN_EVENT_SUCCESS) {
								unsigned long ulDataLength = pEvent->getDataLength();
								printf("\n\tData: (offset=0x%lx, length=0x%lx)\n", pEvent->getDataOffset(), ulDataLength);
						
								unsigned long i,j;
								for (i=0; i<ulDataLength; ) {
									printf("\t   %04ld:\t", i);
									for (j=i; j<ulDataLength && j<i+8; j++) {
										printf("%s%02x", (j==i ? "" : " "), pData[j]);
									}
									for (; j<i+8; j++) {
										printf("   ");
									}
									printf("\t\t");
									for (j=i; j<ulDataLength && j<i+8; j++) {
										if (pData[j] == '\0') {
											printf(" .");
										} else if (pData[j] < 0x20 || pData[j] > 0x7e) {
											printf(" _");
										} else {
											printf(" %c", pData[j]);
										}
									}	//for (j=i; j<ulDataLength && j<i+8; j++) {
									for (; j<i+8; j++) {
										printf("  ");
									}
									printf("\n");
									i = j;
								}	//for (i=0; i<ulDataLength; ) {
								
								free(pData);
								pData = NULL;
							}	//if (pEvent->getData((char**)&pData) == WIN_EVENT_SUCCESS) {
						}	//if (pEvent->getDataLength() > 0) {
						
						printf("----------------------------------------------------------------------------------------------------\n");
					}	//if (displayEvent(pEvent->getEventCode(), &eventsVector) == true) {
					
					delete pEvent;
					pEvent = NULL;
				}	//while (eventFile.getNextRecord(&pEvent) == WIN_EVENT_SUCCESS) {
			}	//else {	//if (bDelimited) {
		}	//if (arguments.bMactime) {
	}	//for (vector<string>::iterator it = arguments.filenameVector.begin(); it != arguments.filenameVector.end(); it++) {
	printf("\nRecord Count: %ld\n", lRecordCount);
		
	exit(rv);	
}	//int main(int argc, const char** argv) {
