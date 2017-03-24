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

// #define _DEBUG_ 1

#include "libWinUsnJrnl/src/winUSNJournal.h"
#include "libWinUsnJrnl/src/winUSNRecord.h"
#include "libtimeUtils/src/timeZoneCalculator.h"
#include "libtimeUtils/src/timeUtils.h"

#include <popt.h>

#include <string>
#include <vector>
using namespace std;

#include "misc/stringType.h"
#include "misc/poptUtils.h"
#include "misc/tsk_mactime.h"
#include "misc/boost_lexical_cast_wrapper.hpp"

#define OPT_DELIMITED		10
#define OPT_START_DATE		20
#define OPT_END_DATE			30
#define OPT_WITH_FILENAME	40
#define OPT_NO_FILENAME		45
#define OPT_MACTIME			50
#define OPT_TIMEZONE			60
#define OPT_VERSION			200

//Sleuthkit TSK3.x body format
//0  |1        |2    |3     |4       |5       |6   |7    |8    |9    |10
//MD5|NAME     |INODE|PERMS |UID     |GID     |SIZE|ATIME|MTIME|CTIME|CRTIME
//
int main(int argc, const char** argv) {
	int rv = EXIT_FAILURE;
	
	vector<string> vectorFilenames;
	bool bDelimited = false;
	string strDateStart;
	string strDateEnd;
	bool bWithFilename = false;
	bool bNoFilename = false;
	bool bMactime = false;
	
	timeZoneCalculator tzcalc;

	//TODO	Another interesting upgrade would be options to find/filter based on filename and/or inode (MFT+Seq). 
	//			This would allow visualizing a "history" of a certain file.
	struct poptOption optionsTable[] = {
		{"delimited", 			'd',	POPT_ARG_NONE,		NULL,	OPT_DELIMITED,			"Display in comma-delimited format.", NULL},
		{"start-date", 		0,		POPT_ARG_STRING,	NULL,	OPT_START_DATE, 		"Only display entries recorded after the specified date.", "mm/dd/yyyy"},
		{"end-date", 		 	0,		POPT_ARG_STRING,	NULL,	OPT_END_DATE, 			"Only display entries recorded before the specified date.", "mm/dd/yyyy"},
		{"with-filename",		'H',	POPT_ARG_NONE,		NULL,	OPT_WITH_FILENAME,	"Display filename in output. Useful when batch processing multiple files.", NULL},
		{"no-filename", 		'h',	POPT_ARG_NONE,		NULL,	OPT_NO_FILENAME,	 	"Suppress filename in output.", NULL},
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
			case OPT_NO_FILENAME:
				bNoFilename = true;
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
		vectorFilenames.push_back(cstrFilename);
		cstrFilename = poptGetArg(optCon);
	}
	
	if (vectorFilenames.size() < 1) {
		usage(optCon, "You must specify at least one file", "e.g., $Extend\\$UsnJrnl:$J");
		exit(EXIT_FAILURE);
	}
	
	if (bDelimited) {
		cout << "Time Zone: \"" << tzcalc.getTimeZoneString() << "\"" << endl;		//Display the timezone so that the reader knows which zone was used for this output
		if ((vectorFilenames.size() > 1 && bNoFilename == false) || bWithFilename == true) {
			cout << "File,";
		}
		cout << "Inode,Parent Inode,USN,Date,Time,Reasons,Sources,Security ID,File Attributes,Filename\n";
	}
	
	long cRecords = 0;
	for (vector<string>::iterator it = vectorFilenames.begin(); it != vectorFilenames.end(); it++) {
		winUSNJournal CjournalFile(*it);

		winUSNRecord* p_clsUSNRecord = NULL;
		while (CjournalFile.getNextRecord(&p_clsUSNRecord) == USNJRNL_SUCCESS) {
			cRecords++;

			DEBUG_INFO("winUsnJrnlViewer p_clsUSNRecord.dtmTimestamp=" << p_clsUSNRecord->getTimestamp());
			string strTime = getTimeString(tzcalc.calculateLocalTime(getFromWindows64(p_clsUSNRecord->getTimestamp())));
			string strDate = getDateString(tzcalc.calculateLocalTime(getFromWindows64(p_clsUSNRecord->getTimestamp())));

			if (bMactime) {
				// TODO 	An interesting upgrade to this project would be to modify whether the timestamp is associated m,a,c,or b
				// 		based on the action noted in the change journal (USN_REASON).
				cout 	<< "|"
						<< p_clsUSNRecord->getFilename() << " (" << p_clsUSNRecord->getReasons() << ")|"
						<< p_clsUSNRecord->getMFTEntry() << " (" << p_clsUSNRecord->getMFTSeq() << ")|"
						<< "winusnjrnl--" << "|"
						<< "|"
						<< "|"
						<< "|"
						<< "|"
						<< getUnix32FromWindows64(p_clsUSNRecord->getTimestamp()) << "|"
						<< "|"
						<< "\n";
			} else {
				if (bDelimited) {
					// File,Inode,Parent Inode,USN,Time,Reasons,Sources,Security ID,File Attributes,Filename
					if ((vectorFilenames.size() > 1 && bNoFilename == false) || bWithFilename == true) {
						cout << it->c_str() << ",";
					}
					cout 	<< p_clsUSNRecord->getMFTEntry() << ","
							<< p_clsUSNRecord->getParentMFTEntry() << ","
							<< p_clsUSNRecord->getUSN() << ","
							<< strDate << ","
							<< strTime << ","
							<< "\"" << p_clsUSNRecord->getReasons() << "\","
							<< "\"" << p_clsUSNRecord->getSources() << "\","
							<< p_clsUSNRecord->getSecurityID() << ","
							<< "\"" << p_clsUSNRecord->getFileAttrs() << "\","
							<< "\"" << p_clsUSNRecord->getFilename() << "\"\n";
				} else {
					if ((vectorFilenames.size() > 1 && bNoFilename == false) || bWithFilename == true) {
						cout << it->c_str() << " ";
					}
					cout	<< "USN " 						<< p_clsUSNRecord->getUSN() << " "
							<< "(offset=" 					<< p_clsUSNRecord->getRecordPos() 		<< ", length=" 	<< p_clsUSNRecord->getRecordLen() << "):\n"
							<< "\tFilename:\t\t"			<< p_clsUSNRecord->getFilename() 		<< "\n"
							<< "\tDate:\t\t\t"			<< strDate										<< "\tTime:\t"		<< strTime << " (" << tzcalc.getTimeZoneString()	<< ")\n"
							<< "\tMFT Entry:\t\t"		<< p_clsUSNRecord->getMFTEntry() 		<< "\tSeq:\t\t" 	<< p_clsUSNRecord->getMFTSeq() << "\n"
							<< "\tParent MFT Entry:\t"	<< p_clsUSNRecord->getParentMFTEntry()	<< "\tSeq:\t\t"	<< p_clsUSNRecord->getParentMFTSeq() << "\n"
							<< "\tUSN Version:\t\t"		<< p_clsUSNRecord->getVersion() 			<< "\n"
							<< "\tSecurity ID:\t\t"		<< p_clsUSNRecord->getSecurityID()		<< "\n"
							<< "\tReasons:\t\t"			<< p_clsUSNRecord->getReasons()			<< "\n"
							<< "\tSources:\t"				<< p_clsUSNRecord->getSources()			<< "\n"
							<< "\tFile Attributes:\t"	<< p_clsUSNRecord->getFileAttrs()		<< "\n"
							<< "----------------------------------------------------------------------------------------------------\n";
				}
			}

			delete p_clsUSNRecord;
			p_clsUSNRecord = NULL;

		} //while (CjournalFile.getNextRecord(&p_clsUSNRecord) == USNJRNL_SUCCESS)

	} //for (vector<string>::iterator it = vectorFilenames.begin(); it != vectorFilenames.end(); it++)

	cout << "Record Count: " << cRecords << "\n";

	exit(rv);	
}	//int main(int argc, const char** argv)
