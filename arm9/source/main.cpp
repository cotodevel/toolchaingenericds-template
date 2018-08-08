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

//C++ part
using namespace std;
#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>

#include "socket.h"
#include "in.h"
#include <netdb.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "main.h"
#include "InterruptsARMCores_h.h"
#include "specific_shared.h"
#include "ff.h"
#include "memoryHandleTGDS.h"
#include "reent.h"
#include "sys/types.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "typedefsTGDS.h"
#include "consoleTGDS.h"
#include "utilsTGDS.h"

#include "devoptab_devices.h"
#include "fsfatlayerTGDSNew.h"
#include "usrsettingsTGDS.h"
#include "exceptionTGDS.h"

#include "videoTGDS.h"
#include "keypadTGDS.h"
#include "guiTGDS.h"
#include "dldi.h"

//test1
//default class instance
class cl {
  int i; // private by default
public:
  int get_i();
  void put_i(int j);
};

int cl::get_i()
{
  return i;
}

void cl::put_i(int j)
{
  i = j;
}

//test2
//constructor example
// Class to represent a box
class Box
{
  public:
    int length = 0;
    int breadth = 0;
    int height = 0;

    // Constructor
    Box(int lengthValue, int breadthValue, int heightValue)
    {
      printf("Box constructor called");
      length = lengthValue;
      breadth = breadthValue;
      height = heightValue;
    }

    // Function to calculate the volume of a box
    int volume()
    {
      return length * breadth * height;
    }
};

//test 3
//class copy
class myclass {
  int *p;
public:
  myclass(int i);
  ~myclass();
  int getval() { return *p; }
};

myclass::myclass(int i)
{
  printf("Allocating p");
  p = new int;
  if(!p) {
    printf("Allocation failure.");
    exit(1); // exit program if out of memory
  }
  *p = i;
}

myclass::~myclass()
{
  printf("Freeing p");
  delete p;
}

// when this function is called, the copy constructor is called
void display(myclass ob)
{
	printf("%d",ob.getval());
}


//test4
class myclass2 {
  int *p;
public:
  myclass2(int i);
  ~myclass2();
  int getval() { return *p; }
};

myclass2::myclass2(int i)
{
	printf("Allocating p");
	p = new int;
	if(!p) {
		printf("Allocation failure.");
		exit(1); // exit program if out of memory
	}
	*p = i;
}

// use destructor to free memory
myclass2::~myclass2()
{
  printf("Freeing p");
  delete p;
}

void display2(myclass2 &ob)
{
	printf("%d",ob.getval());
}

string ToStr( char c ) {
   return string( 1, c );
}

std::string getDldiDefaultPath(){
	std::string dldiOut = string((char*)getfatfsPath( (sint8*)string(dldi_tryingInterface() + string(".dldi")).c_str() ));
	return dldiOut;
}

void menuShow(){
	printf("Select: clearscreen");
	printf("Read File: Y 0:/filelist.txt");
	printf("Write File: rootfileList to 0:/filelist.txt");
	printf("L: dump dldi file from memory");
	printf("	to %s",getDldiDefaultPath().c_str());
	
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

int main(int _argc, sint8 **_argv) {
	
	/*			TGDS 1.4 Standard ARM9 Init code start	*/
	IRQInit();
	
	bool project_specific_console = false;	//set default console or custom console: default console
	GUI_init(project_specific_console);
	GUI_clear();
	
	sint32 fwlanguage = (sint32)getLanguage();
	
	int ret=FS_init();
	if (ret == 0)
	{
		printf("FS Init ok.");
	}
	else if(ret == -1)
	{
		printf("FS Init error.");
	}
	/*			TGDS 1.4 Standard ARM9 Init code end	*/
	
	
	//custom Handler
	setupCustomExceptionHandler((uint32*)&CustomDebugHandler);
	
	menuShow();
	
	while (1)
	{
		/*
		if (keysPressed() & KEY_A){
			printf("mylist should contain:1 10 20 30 30 20 2 3 4 5");
			std::list<int> mylist;
			std::list<int>::iterator it;
			// set some initial values:
			for (int i=1; i<=5; ++i) mylist.push_back(i); // 1 2 3 4 5

			it = mylist.begin();
			++it;       // it points now to number 2           ^

			mylist.insert (it,10);                        // 1 10 2 3 4 5

			// "it" still points to number 2                      ^
			mylist.insert (it,2,20);                      // 1 10 20 20 2 3 4 5

			--it;       // it points now to the second 20            ^

			std::vector<int> myvector (2,30);
			mylist.insert (it,myvector.begin(),myvector.end());
													// 1 10 20 30 30 20 2 3 4 5
													//               ^
			for (it=mylist.begin(); it!=mylist.end(); ++it){
				printf("%d-",*it);
			}
			
			while(keysPressed() & KEY_A){}
		}
		
		if (keysPressed() & KEY_B){
			//	//float printf support: todo
			// sqrtf() is a library function to calculate square root
			//float number = 5.0, squareRoot = 0.0;
			//squareRoot = sqrtf(number);
			//printf("float:Square root of %f=%f",number,squareRoot);
			
			//double number2 = 4.0, squareRoot2 = 0.0;
			//squareRoot = sqrt(number2);
			//printf("double:Square root of %lf=%lf",number,squareRoot);

			cl s;

			s.put_i(10);
			printf("Test1:res:%d",s.get_i());
			
			//test constructor
			Box BoxInst(20,20,80);
			printf("Test2:Box Test:%d",BoxInst.volume());
			
			//copy constructor
			printf("Test3: Constructor");
			myclass a(10);
			display(a);
			
			while(keysPressed() & KEY_B){}
		}
		
		if (keysPressed() & KEY_X){
			printf("Test4: Destructor");
			myclass2 a(10);
			display2(a);
			while(keysPressed() & KEY_X){}
		}
		*/
		if (keysPressed() & KEY_Y){
		
			char InFile[80];  // input file name
			char ch;
			
			ifstream InStream;
			std::string someString;
			
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
			}
			
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
			printf("testfile.txt:%s",someString.c_str());
			while(keysPressed() & KEY_Y){}
		}
		
		if (keysPressed() & KEY_SELECT){
			GUI_clear();
		}
		
		if (keysPressed() & KEY_START){
			std::string filelogout = string(getfatfsPath("filelist.txt"));
			std::ofstream outfile (filelogout,std::ofstream::binary);
			char fname[MAX_TGDSFILENAME_LENGTH+1] = {0};
			int retf = FAT_FindFirstFile(fname);
			while(retf != FT_NONE){
				std::string Filename = string(fname);
				outfile << Filename << endl;
				
				//file?
				retf = FAT_FindNextFile(fname);
				
				//otherwise directory?
				if(retf == FT_DIR){
					FileClass FileClassInst = getEntryFromGlobalListByIndex(LastDirEntry); 
					std::string FullPathStr = buildFullPathFromFileClass(&FileClassInst);
					sprintf(fname,"%s/<dir>",FullPathStr.c_str());
				}			
			}
			outfile.close();
			printf("filelist %s saved.",filelogout.c_str());
			while(keysPressed() & KEY_START){}
		}
		
		if (keysPressed() & KEY_L){
			
			DLDI_INTERFACE* dldiInterface = dldiGet();
			
			uint8 * dldiStart = (uint8 *)dldiInterface;
			int dldiSize = (int)pow((double)2, (double)dldiInterface->driverSize);
			FILE * fh = fopen(getDldiDefaultPath().c_str(),"w+");
			if(fh){
				fwrite(dldiStart, 1, dldiSize, fh);
				fclose(fh);
				printf("%s exported.",getDldiDefaultPath().c_str());
				printf("%d bytes",dldiSize);
			}
			while(keysPressed() & KEY_L){}
		}
		
		IRQVBlankWait();
	}

}