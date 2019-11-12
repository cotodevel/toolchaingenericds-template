/*

			Copyright (C) 2017  Coto
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
USA

*/

#include "main.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "typedefsTGDS.h"
#include "gui_console_connector.h"
#include "dswnifi_lib.h"
#include "dldi.h"
#include "TGDSNDSLogo.h"
#include "fileBrowse.hpp"	//generic template functions from TGDS: maintain 1 source, whose changes are globally accepted by all TGDS Projects.

//C++ part
using namespace std;
#include <vector>
#include <fstream>
#include <cmath>

char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];

std::string getDldiDefaultPath(){
	std::string dldiOut = string((char*)getfatfsPath( (sint8*)string(dldi_tryingInterface() + string(".dldi")).c_str() ));
	return dldiOut;
}

void menuShow(){
	clrscr();
	printf("                              ");
	printf("Y: Read File: 0:/filelist.txt");
	printf("X: generate root file list into 0:/filelist.txt");
	printf("L: dump dldi file to %s",getDldiDefaultPath().c_str());
	printf("Start: simple file browser");
	printf("Select: this menu");
}

//customHandler 
void CustomDebugHandler(){
	clrscr();
	
	//Init default console here
	bool project_specific_console = false;	//set default console or custom console: default console
	GUI_init(project_specific_console);
	GUI_clear();
	
	printf("custom Handler!");
	uint32 * debugVector = (uint32 *)&exceptionArmRegs[0];
	uint32 pc_abort = (uint32)exceptionArmRegs[0xf];
	
	if((debugVector[0xe] & 0x1f) == 0x17){
		pc_abort = pc_abort - 8;
	}
	
	printf("R0[%x] R1[%X] R2[%X] \n",debugVector[0],debugVector[1],debugVector[2]);
	printf("R3[%x] R4[%X] R5[%X] \n",debugVector[3],debugVector[4],debugVector[5]);
	printf("R6[%x] R7[%X] R8[%X] \n",debugVector[6],debugVector[7],debugVector[8]);
	printf("R9[%x] R10[%X] R11[%X] \n",debugVector[9],debugVector[0xa],debugVector[0xb]);
	printf("R12[%x] R13[%X] R14[%X]  \n",debugVector[0xc],debugVector[0xd],debugVector[0xe]);
	printf("R15[%x] SPSR[%x] CPSR[%X]  \n",pc_abort,debugVector[17],debugVector[16]);
	while(1==1){}
}

vector<string> splitCustom(string str, string token){
    vector<string>result;
    while(str.size()){
        int index = str.find(token);
        if(index != (int)string::npos){
            result.push_back(str.substr(0,index));
            str = str.substr(index+token.size());
            if(str.size()==0)result.push_back(str);
        }else{
            result.push_back(str);
            str = "";
        }
    }
    return result;
}

int main(int _argc, sint8 **_argv) {
	
	/*			TGDS 1.5 Standard ARM9 Init code start	*/
	bool project_specific_console = true;	//set default console or custom console: custom console
	GUI_init(project_specific_console);
	GUI_clear();
	
	sint32 fwlanguage = (sint32)getLanguage();
	
	printf("     ");
	printf("     ");
	
	int ret=FS_init();
	if (ret == 0)
	{
		printf("FS Init ok.");
	}
	else if(ret == -1)
	{
		printf("FS Init error.");
	}
	switch_dswnifi_mode(dswifi_idlemode);
	/*			TGDS 1.5 Standard ARM9 Init code end	*/
	
	//custom Handler
	setupCustomExceptionHandler((uint32*)&CustomDebugHandler);
	menuShow();
	
	//load TGDS Logo (NDS BMP Image)
	//VRAM A Used by console
	//VRAM C Keyboard and/or TGDS Logo
	
	printf("BMPBG is at: %x", &TGDSLogoNDSSize);
	initFBModeSubEngine0x06200000();
	renderFBMode3SubEngine((u16*)&TGDSLogoNDSSize[0], (int)TGDSLOGONDSSIZE_WIDTH,(int)TGDSLOGONDSSIZE_HEIGHT);
	
	while (1){
		scanKeys();
		
		if (keysPressed() & KEY_START){
			char startPath[MAX_TGDSFILENAME_LENGTH+1];
			sprintf(startPath,"%s","/");
			while( ShowBrowser((char *)startPath, (char *)&curChosenBrowseFile[0]) == true ){	//as long you keep using directories ShowBrowser will be true
				
			}
			while(keysPressed() & KEY_START){
				scanKeys();
			}
			menuShow();
		}
		
		if (keysPressed() & KEY_Y){
		
			char InFile[80];  // input file name
			char ch;
			
			ifstream InStream;
			std::string someString;
			
			ofstream OutStream;
			
			
			sprintf(InFile,"%s%s",getfatfsPath((char*)""),"filelist.txt");
			
			// Open file for input
			// in.open(fin); also works
			InStream.open(InFile, ios::in);

			// ensure file is opened successfully
			if(!InStream)
			{
				printf("Error open file %s",InFile);
				//return 1;
			}
			else{
				printf("OK open file %s",InFile);
				
				// Read in each character until eof character is read.
				// Output it to screen.
				int somePosition = 0;
				while (!InStream.eof()) {
					//Read each character.
					InStream.get(ch);
					someString.insert(somePosition, ToStr(ch));
					somePosition++;
				}
				InStream.close();
				printf("->%s",someString.c_str());
			}
			
			while(keysPressed() & KEY_Y){
				scanKeys();
			}
		}
		
		if (keysPressed() & KEY_SELECT){
			menuShow();
		}
		
		if (keysPressed() & KEY_X){
			
			string fOut = string(getfatfsPath((char *)"filelist.txt"));
			std::ofstream outfile;
			outfile.open(fOut.c_str());
			char fname[MAX_TGDSFILENAME_LENGTH+1] = {0};
			int retf = FAT_FindFirstFile(fname);
			while(retf != FT_NONE){
				std::string fnameOut = std::string("");
				//directory?
				if(retf == FT_DIR){
					struct FileClass * fileClassInst = getFileClassFromList(LastDirEntry);
					std::string outDirName = string(fileClassInst->fd_namefullPath);
					fnameOut = parseDirNameTGDS(outDirName) + string("/<dir>");
				}
				//file?
				else if(retf == FT_FILE){
					std::string outfileName = string(fname);
					fnameOut = parsefileNameTGDS(outfileName);
				}
				outfile << fnameOut << endl;
				
				//more file/dir objects?
				retf = FAT_FindNextFile(fname);
			}
			
			outfile.close();
			printf("filelist %s saved.", fOut.c_str());
			while(keysPressed() & KEY_X){
				scanKeys();
			}
		}
		
		if (keysPressed() & KEY_L){
			DLDI_INTERFACE* dldiInterface = dldiGet();
			uint8 * dldiStart = (uint8 *)dldiInterface;
			int dldiSize = (int)pow((double)2, (double)dldiInterface->driverSize);	// this is easily 2 << (dldiInterface->driverSize - 1), but I use pow() to test the math library in the ARM9 core
			FILE * fh = fopen(getDldiDefaultPath().c_str(),"w+");
			if(fh){
				fwrite(dldiStart, 1, dldiSize, fh);
				fclose(fh);
				printf("%s exported.",getDldiDefaultPath().c_str());
				printf("%d bytes",dldiSize);
			}
			while(keysPressed() & KEY_L){
				scanKeys();
			}
		}
		
		IRQVBlankWait();
	}

}