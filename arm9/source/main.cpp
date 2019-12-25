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
#include "ipcfifoTGDS.h"
#include "fileBrowse.h"	//generic template functions from TGDS: maintain 1 source, whose changes are globally accepted by all TGDS Projects.
#include "utilTGDSTemplate9.h"
#include "TGDSLogoLZSSCompressed.h"
#include "nds_cp15_misc.h"
#include "click_raw.h"
#include "soundTGDS.h"

//C++ part
using namespace std;
#include <vector>
#include <fstream>
#include <cmath>

char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];

static inline string ToStr( char c ) {
   return string( 1, c );
}

//example: std::string OverrideFileExtension("filename.txt", ".zip")
static inline std::string OverrideFileExtension(const std::string& FileName, const std::string& newExt)
{
	std::string newString = string(FileName);
    if(newString.find_last_of(".") != std::string::npos)
        return newString.substr(0,newString.find_last_of('.'))+newExt;
    return "";
}

static inline void waitByLoopAButton(){
	scanKeys();
	while(1==1){
		if(keysPressed()&KEY_A){
			break;
		}
		scanKeys();
		IRQWait(IRQ_VBLANK);
	}
}

static inline bool ShowBrowserCustom(char * Path, char * outBuf){
	scanKeys();
	while((keysPressed() & KEY_START) || (keysPressed() & KEY_A) || (keysPressed() & KEY_B)){
		scanKeys();
		IRQWait(IRQ_VBLANK);
	}
	int pressed = 0;
	vector<struct FileClass *> internalName;	//required to handle FILE/DIR types from TGDS FS quick and easy
	struct FileClass filStub;
	char fname[256+1];
	sprintf(fname,"%s",Path);
	int j = 1;
    
	//OK, use the new CWD and build the playlist
	internalName.push_back(&filStub);
	
	//Create TGDS Dir API context
	struct FileClassList * fileClassListCtx = initFileList();
	cleanFileList(fileClassListCtx);
	
	int startFromIndex = 0;
	struct FileClass * fileClassInst = NULL;
	fileClassInst = FAT_FindFirstFile(fname, fileClassListCtx, startFromIndex);
	while(fileClassInst != NULL){
		//directory?
		if(fileClassInst->type == FT_DIR){
			char tmpBuf[MAX_TGDSFILENAME_LENGTH+1];
			strcpy(tmpBuf, fname);
			parseDirNameTGDS(tmpBuf);
			strcpy(fileClassInst->fd_namefullPath, tmpBuf);
		}
		//file?
		else if(fileClassInst->type == FT_FILE){
			char tmpBuf[MAX_TGDSFILENAME_LENGTH+1];
			strcpy(tmpBuf, fname);
			parsefileNameTGDS(tmpBuf);
			strcpy(fileClassInst->fd_namefullPath, tmpBuf);
		}
		internalName.push_back(fileClassInst);
		
		//more file/dir objects?
		fileClassInst = FAT_FindNextFile(fname, fileClassListCtx);
	}
	
	//actual file lister
	clrscr();
	
	j = 1;
	pressed = 0 ;
	int lastVal = 0;
	bool reloadDirA = false;
	bool reloadDirB = false;
	std::string newDir = std::string("");
	
	#define itemsShown (int)(15)
	int curjoffset = 0;
	int itemRead=1;
	
	while(1){
		
		int itemsToLoad = (internalName.size() - curjoffset);
		
		//check if remaining items are enough
		if(itemsToLoad > itemsShown){
			itemsToLoad = itemsShown;
		}
		
		while(itemRead < itemsToLoad ){
			std::string strDirFileName = string(internalName.at(itemRead+curjoffset)->fd_namefullPath);		
			if(internalName.at(itemRead+curjoffset)->type == FT_DIR){
				printfCoords(0, itemRead, "--- %s%s",strDirFileName.c_str(),"<dir>");
			}
			else{
				printfCoords(0, itemRead, "--- %s",strDirFileName.c_str());
			}
			itemRead++;
		}
		
		scanKeys();
		pressed = keysPressed();
		if (pressed&KEY_DOWN && (j < (itemsToLoad - 1) ) ){
			j++;
			while(pressed&KEY_DOWN){
				scanKeys();
				pressed = keysPressed();
				IRQWait(IRQ_VBLANK);
			}
		}
		
		//downwards: means we need to reload new screen
		else if(pressed&KEY_DOWN && (j >= (itemsToLoad - 1) ) && ((internalName.size() - curjoffset - itemRead) > 0) ){
			
			//list only the remaining items
			clrscr();
			
			curjoffset = (curjoffset + itemsToLoad - 1);
			itemRead = 1;
			j = 1;
			
			scanKeys();
			pressed = keysPressed();
			while(pressed&KEY_DOWN){
				scanKeys();
				pressed = keysPressed();
				IRQWait(IRQ_VBLANK);
			}
		}
		
		//LEFT, reload new screen
		else if(pressed&KEY_LEFT && ((curjoffset - itemsToLoad) > 0) ){
			
			//list only the remaining items
			clrscr();
			
			curjoffset = (curjoffset - itemsToLoad - 1);
			itemRead = 1;
			j = 1;
			
			scanKeys();
			pressed = keysPressed();
			while(pressed&KEY_LEFT){
				scanKeys();
				pressed = keysPressed();
				IRQWait(IRQ_VBLANK);
			}
		}
		
		//RIGHT, reload new screen
		else if(pressed&KEY_RIGHT && ((internalName.size() - curjoffset - itemsToLoad) > 0) ){
			
			//list only the remaining items
			clrscr();
			
			curjoffset = (curjoffset + itemsToLoad - 1);
			itemRead = 1;
			j = 1;
			
			scanKeys();
			pressed = keysPressed();
			while(pressed&KEY_RIGHT){
				scanKeys();
				pressed = keysPressed();
				IRQWait(IRQ_VBLANK);
			}
		}
		
		else if (pressed&KEY_UP && (j > 1)) {
			j--;
			while(pressed&KEY_UP){
				scanKeys();
				pressed = keysPressed();
				IRQWait(IRQ_VBLANK);
			}
		}
		
		//upwards: means we need to reload new screen
		else if (pressed&KEY_UP && (j <= 1) && (curjoffset > 0) ) {
			//list only the remaining items
			clrscr();
			
			curjoffset--;
			itemRead = 1;
			j = 1;
			
			scanKeys();
			pressed = keysPressed();
			while(pressed&KEY_UP){
				scanKeys();
				pressed = keysPressed();
				IRQWait(IRQ_VBLANK);
			}
		}
		
		//reload DIR (forward)
		else if( (pressed&KEY_A) && (internalName.at(j+curjoffset)->type == FT_DIR) ){
			newDir = string(internalName.at(j+curjoffset)->fd_namefullPath);
			reloadDirA = true;
			break;
		}
		
		//file chosen
		else if( (pressed&KEY_A) && (internalName.at(j+curjoffset)->type == FT_FILE) ){
			break;
		}
		
		//reload DIR (backward)
		else if(pressed&KEY_B){
			reloadDirB = true;
			break;
		}
		
		// Show cursor
		printfCoords(0, j, "*");
		if(lastVal != j){
			printfCoords(0, lastVal, " ");	//clean old
		}
		lastVal = j;
	}
	
	//enter a dir
	if(reloadDirA == true){
		internalName.clear();
		
		//Free TGDS Dir API context
		freeFileList(fileClassListCtx);
		
		enterDir((char*)newDir.c_str());
		return true;
	}
	
	//leave a dir
	if(reloadDirB == true){
	
		//Free TGDS Dir API context
		freeFileList(fileClassListCtx);
		
		//rewind to preceding dir in TGDSCurrentWorkingDirectory
		leaveDir(getTGDSCurrentWorkingDirectory());
		return true;
	}
	
	strcpy((char*)outBuf, internalName.at(j+curjoffset)->fd_namefullPath);
	clrscr();
	printf("                                   ");
	if(internalName.at(j+curjoffset)->type == FT_DIR){
		
	}
	else if(internalName.at(j+curjoffset)->type == FT_FILE){
		string filenameChosen = string(internalName.at(j+curjoffset)->fd_namefullPath);
		string targetLZSSFilenameOut = OverrideFileExtension(filenameChosen, ".lzss");
		string targetLZSSFilenameIn = OverrideFileExtension(filenameChosen, ".bin");
		printf("Press X to LZSS compress");
		printf("%s into %s",filenameChosen.c_str(), targetLZSSFilenameOut.c_str());
		printf("");
		printf("Press Y to LZSS decompress");
		printf("%s into %s",filenameChosen.c_str(), targetLZSSFilenameIn.c_str());
		printf("");
		printf("Press B to exit");
		
		scanKeys();
		while(1==1){
			if(keysPressed()&KEY_X){
				//compress
				printf("Compressing please wait...");
				LZS_Encode(filenameChosen.c_str(), targetLZSSFilenameOut.c_str());
				printf("Press A to exit");
				waitByLoopAButton();
				break;
			}
			else if(keysPressed()&KEY_Y){
				//decompress
				printf("Decompressing please wait...");
				LZS_Decode(filenameChosen.c_str(), targetLZSSFilenameIn.c_str());
				printf("Press A to exit");
				waitByLoopAButton();
				break;
			}
			else if(keysPressed()&KEY_B){
				break;
			}
			scanKeys();
			IRQWait(IRQ_VBLANK);
		}
	}
	
	//Free TGDS Dir API context
	freeFileList(fileClassListCtx);
	
	return false;
}

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
	printf("D-PAD Left: test IPC Irqs");
	printf("D-PAD Down: Sound click test ");
	
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

struct sSharedSENDCtx SENDCtxInst;

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
	
	//render TGDSLogo from a LZSS compressed file
	RenderTGDSLogoSubEngine((uint8*)&TGDSLogoLZSSCompressed[0], TGDSLogoLZSSCompressed_size);
	
	
	
	/*
	printf("SendBufferThroughFIFOIrqsAsync() start.");
	u32 * buf = (u32*)0x02000000;
	struct sSharedSENDCtx * uncachedCtx = (struct sSharedSENDCtx *) ( ((u32)&SENDCtxInst) + 0x400000);

	SendBufferThroughFIFOIrqsAsync((u32)0x03800000, (u32)buf, 64*4, uncachedCtx);
	printf("SendBufferThroughFIFOIrqsAsync() done.");
	printf("%x[%x] - %x[%x]", (u32)&buf[1], *((u32*)&buf[1]), (u32)(&buf[4]) , *((u32*)&buf[4]) );
	printf("%x[%x] - %x[%x]", (u32)&buf[0xc], *((u32*)&buf[0xc]), (u32)(&buf[0x10]) , *((u32*)&buf[0x10]) );
	
	while(1==1);
	*/
	
	while (1){
		scanKeys();
		
		if (keysPressed() & KEY_START){
			char startPath[MAX_TGDSFILENAME_LENGTH+1];
			sprintf(startPath,"%s","/");
			while( ShowBrowserCustom((char *)startPath, (char *)&curChosenBrowseFile[0]) == true ){	//as long you keep using directories ShowBrowser will be true
				
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
			
			//Create TGDS Dir API context
			struct FileClassList * fileClassListCtx = initFileList();
			cleanFileList(fileClassListCtx);
			
			//Use TGDS Dir API context
			int startFromIndex = 0;
			struct FileClass * fileClassInst = NULL;
			fileClassInst = FAT_FindFirstFile(fname, fileClassListCtx, startFromIndex);			
			while(fileClassInst != NULL){
				std::string fnameOut = std::string("");
				//directory?
				if(fileClassInst->type == FT_DIR){
					char tmpBuf[MAX_TGDSFILENAME_LENGTH+1];
					strcpy(tmpBuf, fname);
					parseDirNameTGDS(tmpBuf);					
					fnameOut = string(tmpBuf) + string("/<dir>");
				}
				//file?
				else if(fileClassInst->type == FT_FILE){
					char tmpBuf[MAX_TGDSFILENAME_LENGTH+1];
					strcpy(tmpBuf, fname);
					parsefileNameTGDS(tmpBuf);					
					fnameOut = string(tmpBuf);
				}
				outfile << fnameOut << endl;
				
				//more file/dir objects?
				fileClassInst = FAT_FindNextFile(fname, fileClassListCtx);
			}
			
			//Free TGDS Dir API context
			freeFileList(fileClassListCtx);
			
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
		
		
		if (keysPressed() & KEY_LEFT){
			sendByteIPC(READ_EXTARM_IPC);	//should trigger a IPC IRQ
			while(keysPressed() & KEY_LEFT){
				scanKeys();
			}
		}
		
		if (keysPressed() & KEY_DOWN){
			
			u8 channel = 0;	//-1 == auto allocate any channel in the 0--15 range
			setSoundSampleContext(11025, (u32*)&click_raw[0], click_raw_size, channel, 40, 63, 1);	//PCM16 sample
			
			while(keysPressed() & KEY_DOWN){
				scanKeys();
			}
		}
		
		
		IRQVBlankWait();
	}

}