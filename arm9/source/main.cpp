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
#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dswnifi_lib.h"
#include "keypadTGDS.h"
#include "TGDSLogoLZSSCompressed.h"
#include "fileBrowse.h"	//generic template functions from TGDS: maintain 1 source, whose changes are globally accepted by all TGDS Projects.
#include "biosTGDS.h"
#include "ipcfifoTGDSUser.h"
#include "dldi.h"
#include "global_settings.h"
#include "posixHandleTGDS.h"
#include "TGDSMemoryAllocator.h"
#include "consoleTGDS.h"
#include "soundTGDS.h"
#include "nds_cp15_misc.h"
#include "fatfslayerTGDS.h"
#include "utilsTGDS.h"
#include "click_raw.h"
#include "ima_adpcm.h"

struct FileClassList * menuIteratorfileClassListCtx = NULL;
char curChosenBrowseFile[256+1];
char globalPath[MAX_TGDSFILENAME_LENGTH+1];
int internalCodecType = SRC_NONE;//Internal because WAV raw decompressed buffers are used if Uncompressed WAV or ADPCM

static int curFileIndex = 0;
static bool pendingPlay = false;

int physFh1 = -1;	//File Handle 1
int physFh2 = -1;	//File Handle 2

static inline void stopStream(){
	//stop adpcm / wav playback
	struct fd * fd1 = getStructFD(physFh1);
	struct fd * fd2 = getStructFD(physFh2); 
	bool ret = stopSoundStream(fd1, fd2, &internalCodecType);
}

static inline void menuShow(){
	clrscr();
	printf("     ");
	printf("     ");
	printf("toolchaingenericds-template: ");
	printf("Current file: %s ", curChosenBrowseFile);
	printf("(Select): This menu. ");
	printf("(Start): FileBrowser : (A) Play WAV/IMA-ADPCM (Intel) strm ");
	printf("(D-PAD:UP/DOWN): Volume + / - ");
	printf("(D-PAD:LEFT): GDB Debugging. >%d", TGDSPrintfColor_Green);
	printf("(D-PAD:RIGHT): Demo Sound. >%d", TGDSPrintfColor_Yellow);
	printf("(B): Stop WAV/IMA-ADPCM file. ");
	printf("Current Volume: %d", (int)getVolume());
	if(internalCodecType == SRC_WAVADPCM){
		printf("ADPCM Play: %s >%d", curChosenBrowseFile, TGDSPrintfColor_Red);
	}
	else if(internalCodecType == SRC_WAV){	
		printf("WAVPCM Play: %s >%d", curChosenBrowseFile, TGDSPrintfColor_Green);
	}
	else{
		printf("Player Inactive");
	}
	printf("(B): stop .WAV playback ");
	printf("Available heap memory: %d >%d", getMaxRam(), TGDSPrintfColor_Cyan);
	printarm7DebugBuffer();
}

static bool ShowBrowserC(char * Path, char * outBuf, bool * pendingPlay, int * curFileIdx){	//MUST be same as the template one at "fileBrowse.h" but added some custom code
	scanKeys();
	while((keysPressed() & KEY_START) || (keysPressed() & KEY_A) || (keysPressed() & KEY_B)){
		scanKeys();
		IRQWait(IRQ_HBLANK);
	}
	
	*pendingPlay = false;
	
	//Create TGDS Dir API context
	cleanFileList(menuIteratorfileClassListCtx);
	
	//Use TGDS Dir API context
	int pressed = 0;
	struct FileClass filStub;
	{
		filStub.type = FT_FILE;
		strcpy(filStub.fd_namefullPath, "");
		filStub.isIterable = true;
		filStub.d_ino = -1;
		filStub.parentFileClassList = menuIteratorfileClassListCtx;
	}
	char curPath[MAX_TGDSFILENAME_LENGTH+1];
	strcpy(curPath, Path);
	
	if(pushEntryToFileClassList(true, filStub.fd_namefullPath, filStub.type, -1, menuIteratorfileClassListCtx) != NULL){
		
	}
	
	int j = 1;
	int startFromIndex = 1;
	*curFileIdx = startFromIndex;
	struct FileClass * fileClassInst = NULL;
	fileClassInst = FAT_FindFirstFile(curPath, menuIteratorfileClassListCtx, startFromIndex);
	while(fileClassInst != NULL){
		//directory?
		if(fileClassInst->type == FT_DIR){
			char tmpBuf[MAX_TGDSFILENAME_LENGTH+1];
			strcpy(tmpBuf, fileClassInst->fd_namefullPath);
			parseDirNameTGDS(tmpBuf);
			strcpy(fileClassInst->fd_namefullPath, tmpBuf);
		}
		//file?
		else if(fileClassInst->type  == FT_FILE){
			char tmpBuf[MAX_TGDSFILENAME_LENGTH+1];
			strcpy(tmpBuf, fileClassInst->fd_namefullPath);
			parsefileNameTGDS(tmpBuf);
			strcpy(fileClassInst->fd_namefullPath, tmpBuf);
		}
		
		
		//more file/dir objects?
		fileClassInst = FAT_FindNextFile(curPath, menuIteratorfileClassListCtx);
	}
	
	//actual file lister
	clrscr();
	
	j = 1;
	pressed = 0 ;
	int lastVal = 0;
	bool reloadDirA = false;
	bool reloadDirB = false;
	char * newDir = NULL;
	
	#define itemsShown (int)(15)
	int curjoffset = 0;
	int itemRead=1;
	
	while(1){
		int fileClassListSize = getCurrentDirectoryCount(menuIteratorfileClassListCtx) + 1;	//+1 the stub
		int itemsToLoad = (fileClassListSize - curjoffset);
		
		//check if remaining items are enough
		if(itemsToLoad > itemsShown){
			itemsToLoad = itemsShown;
		}
		
		while(itemRead < itemsToLoad ){		
			if(getFileClassFromList(itemRead+curjoffset, menuIteratorfileClassListCtx)->type == FT_DIR){
				printfCoords(0, itemRead, "--- %s%s",getFileClassFromList(itemRead+curjoffset, menuIteratorfileClassListCtx)->fd_namefullPath,"<dir>");
			}
			else{
				printfCoords(0, itemRead, "--- %s",getFileClassFromList(itemRead+curjoffset, menuIteratorfileClassListCtx)->fd_namefullPath);
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
				IRQWait(IRQ_HBLANK);
			}
		}
		
		//downwards: means we need to reload new screen
		else if(pressed&KEY_DOWN && (j >= (itemsToLoad - 1) ) && ((fileClassListSize - curjoffset - itemRead) > 0) ){
			
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
				IRQWait(IRQ_HBLANK);
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
				IRQWait(IRQ_HBLANK);
			}
		}
		
		//RIGHT, reload new screen
		else if(pressed&KEY_RIGHT && ((fileClassListSize - curjoffset - itemsToLoad) > 0) ){
			
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
				IRQWait(IRQ_HBLANK);
			}
		}
		
		else if (pressed&KEY_UP && (j > 1)) {
			j--;
			while(pressed&KEY_UP){
				scanKeys();
				pressed = keysPressed();
				IRQWait(IRQ_HBLANK);
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
				IRQWait(IRQ_HBLANK);
			}
		}
		
		//reload DIR (forward)
		else if( (pressed&KEY_A) && (getFileClassFromList(j+curjoffset, menuIteratorfileClassListCtx)->type == FT_DIR) ){
			struct FileClass * fileClassChosen = getFileClassFromList(j+curjoffset, menuIteratorfileClassListCtx);
			newDir = fileClassChosen->fd_namefullPath;
			reloadDirA = true;
			break;
		}
		
		//file chosen
		else if( (pressed&KEY_A) && (getFileClassFromList(j+curjoffset, menuIteratorfileClassListCtx)->type == FT_FILE) ){
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
		//Free TGDS Dir API context
		//freeFileList(menuIteratorfileClassListCtx);	//can't because we keep the menuIteratorfileClassListCtx handle across folders
		
		enterDir((char*)newDir, Path);
		return true;
	}
	
	//leave a dir
	if(reloadDirB == true){
		//Free TGDS Dir API context
		//freeFileList(menuIteratorfileClassListCtx);	//can't because we keep the menuIteratorfileClassListCtx handle across folders
		
		//rewind to preceding dir in TGDSCurrentWorkingDirectory
		leaveDir(Path);
		return true;
	}
	
	strcpy((char*)outBuf, getFileClassFromList(j+curjoffset, menuIteratorfileClassListCtx)->fd_namefullPath);
	clrscr();
	printf("                                   ");
	if(getFileClassFromList(j+curjoffset, menuIteratorfileClassListCtx)->type == FT_DIR){
		//printf("you chose Dir:%s",outBuf);
	}
	else{
		*curFileIdx = (j+curjoffset);
		*pendingPlay = true;
	}
	
	//Free TGDS Dir API context
	//freeFileList(menuIteratorfileClassListCtx);
	return false;
}


int main(int argc, char argv[argvItems][MAX_TGDSFILENAME_LENGTH]) {
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = false;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	
	printf("              ");
	printf("              ");
	
	bool isCustomTGDSMalloc = false;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(TGDS_ARM7_MALLOCSTART, TGDS_ARM7_MALLOCSIZE, isCustomTGDSMalloc));
	sint32 fwlanguage = (sint32)getLanguage();
	
	#ifdef ARM7_DLDI
	setDLDIARM7Address((u32 *)TGDSDLDI_ARM7_ADDRESS);	//Required by ARM7DLDI!
	#endif
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
	asm("mcr	p15, 0, r0, c7, c10, 4");
	flush_icache_all();
	flush_dcache_all();
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	//Show logo
	RenderTGDSLogoMainEngine((uint8*)&TGDSLogoLZSSCompressed[0], TGDSLogoLZSSCompressed_size);
	
	//Init TGDS FS Directory Iterator Context(s). Mandatory to init them like this!! Otherwise several functions won't work correctly.
	menuIteratorfileClassListCtx = initFileList();
	cleanFileList(menuIteratorfileClassListCtx);
	
	memset(globalPath, 0, sizeof(globalPath));
	strcpy(globalPath,"/");
	
	menuShow();
	
	static bool GDBEnabled = false;
	while(1) {
		if(pendingPlay == true){
			internalCodecType = playSoundStream(curChosenBrowseFile);
			if(internalCodecType == SRC_NONE){
				stopStream();
			}
			pendingPlay = false;
			menuShow();
		}
		
		scanKeys();
		
		if (keysPressed() & KEY_TOUCH){
			u8 channel = SOUNDSTREAM_FREE_CHANNEL;
			startSoundSample(11025, (u32*)&click_raw[0], click_raw_size, channel, 40, 63, 1);	//PCM16 sample
			while(keysPressed() & KEY_TOUCH){
				scanKeys();
			}
		}
		
		if (keysPressed() & KEY_SELECT){
			menuShow();
			while(keysPressed() & KEY_SELECT){
				scanKeys();
			}
		}
		
		if (keysPressed() & KEY_START){
			while( ShowBrowserC((char *)globalPath, (char *)&curChosenBrowseFile[0], &pendingPlay, &curFileIndex) == true ){	//as long you keep using directories ShowBrowser will be true
				
			}
			
			if(getCurrentDirectoryCount(menuIteratorfileClassListCtx) > 0){
				strcpy(curChosenBrowseFile, (const char *)getFileClassFromList(curFileIndex, menuIteratorfileClassListCtx)->fd_namefullPath);
				pendingPlay = true;
			}
			
			scanKeys();
			while(keysPressed() & KEY_START){
				scanKeys();
			}
			menuShow();
		}
		
		if (keysPressed() & KEY_L){
			struct sIPCSharedTGDS * TGDSIPC = TGDSIPCStartAddress;
			if(getCurrentDirectoryCount(menuIteratorfileClassListCtx) > 0){
				if(curFileIndex > 1){	//+1 stub FileClass
					curFileIndex--;
				}
				struct FileClass * fileClassInst = getFileClassFromList(curFileIndex, menuIteratorfileClassListCtx);
				if(fileClassInst->type == FT_FILE){
					strcpy(curChosenBrowseFile, (const char *)fileClassInst->fd_namefullPath);
					pendingPlay = true;
				}
				else{
					strcpy(curChosenBrowseFile, (const char *)fileClassInst->fd_namefullPath);
				}
			}
			
			scanKeys();
			while(keysPressed() & KEY_L){
				scanKeys();
				IRQWait(IRQ_HBLANK);
			}
			menuShow();
		}
		
		if (keysPressed() & KEY_R){	
			//Play next song from current folder
			int lstSize = getCurrentDirectoryCount(menuIteratorfileClassListCtx);
			if(lstSize > 0){	
				if(curFileIndex < lstSize){
					curFileIndex++;
				}
				struct FileClass * fileClassInst = getFileClassFromList(curFileIndex, menuIteratorfileClassListCtx);
				if(fileClassInst->type == FT_FILE){
					strcpy(curChosenBrowseFile, (const char *)fileClassInst->fd_namefullPath);
					pendingPlay = true;
				}
				else{
					strcpy(curChosenBrowseFile, (const char *)fileClassInst->fd_namefullPath);
				}
			}
			
			scanKeys();
			while(keysPressed() & KEY_R){
				scanKeys();
				IRQWait(IRQ_HBLANK);
			}
			menuShow();
		}
		
		if (keysPressed() & KEY_UP){
			struct touchScr touchScrInst;
			touchScrRead(&touchScrInst);
			volumeUp(touchScrInst.touchXpx, touchScrInst.touchYpx);
			menuShow();
			scanKeys();
			while(keysPressed() & KEY_UP){
				scanKeys();
				IRQWait(IRQ_HBLANK);
			}
		}
		
		if (keysPressed() & KEY_DOWN){
			struct touchScr touchScrInst;
			touchScrRead(&touchScrInst);
			volumeDown(touchScrInst.touchXpx, touchScrInst.touchYpx);
			menuShow();
			scanKeys();
			while(keysPressed() & KEY_DOWN){
				scanKeys();
				IRQWait(IRQ_HBLANK);
			}
		}
		
		if (keysPressed() & KEY_B){
			scanKeys();
			stopStream();
			menuShow();
			while(keysPressed() & KEY_B){
				scanKeys();
			}
		}
		
		if (keysPressed() & KEY_LEFT){
			if(GDBEnabled == false){
				GDBEnabled = true;
			}
			else{
				GDBEnabled = false;
			}
			
			while(keysPressed() & KEY_LEFT){
				scanKeys();
			}
		}
		
		if (keysPressed() & KEY_RIGHT){
			strcpy(curChosenBrowseFile, (const char *)"0:/rain.ima");
			pendingPlay = true;
			
			scanKeys();
			while(keysPressed() & KEY_RIGHT){
				scanKeys();
			}
		}
		
		//GDB Debug Start
		
		//GDB Stub Process must run here
		if(GDBEnabled == true){
			int retGDBVal = remoteStubMain();
			if(retGDBVal == remoteStubMainWIFINotConnected){
				if (switch_dswnifi_mode(dswifi_gdbstubmode) == true){
					clrscr();
					//Show IP and port here
					printf("    ");
					printf("    ");
					printf("[Connect to GDB]: NDSMemory Mode!");
					char IP[16];
					printf("Port:%d GDB IP:%s",remotePort, print_ip((uint32)Wifi_GetIP(), IP));
					remoteInit();
				}
				else{
					//GDB Client Reconnect:ERROR
				}
			}
			else if(retGDBVal == remoteStubMainWIFIConnectedGDBDisconnected){
				setWIFISetup(false);
				clrscr();
				printf("    ");
				printf("    ");
				printf("Remote GDB Client disconnected. ");
				printf("Press A to retry this GDB Session. ");
				printf("Press B to reboot NDS GDB Server ");
				
				int keys = 0;
				while(1){
					scanKeys();
					keys = keysPressed();
					if (keys&KEY_A){
						break;
					}
					if (keys&KEY_B){
						break;
					}
					IRQWait(IRQ_HBLANK);
				}
				
				if (keys&KEY_B){
					main(argc, argv);
				}
				
				if (switch_dswnifi_mode(dswifi_gdbstubmode) == true){ // gdbNdsStart() called
					reconnectCount++;
					clrscr();
					//Show IP and port here
					printf("    ");
					printf("    ");
					printf("[Re-Connect to GDB]: NDSMemory Mode!");
					char IP[16];
					printf("Retries: %d",reconnectCount);
					printf("Port:%d GDB IP:%s", remotePort, print_ip((uint32)Wifi_GetIP(), IP));
					remoteInit();
				}
			}
		}
		//GDB Debug End
		
		handleARM9SVC();	/* Do not remove, handles TGDS services */
		IRQWait(IRQ_HBLANK);
	}

	return 0;
}