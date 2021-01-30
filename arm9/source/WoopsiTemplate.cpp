// Includes
#include "WoopsiTemplate.h"
#include "woopsiheaders.h"
#include "bitmapwrapper.h"
#include "bitmap.h"
#include "graphics.h"
#include "rect.h"
#include "gadgetstyle.h"
#include "fonts/newtopaz.h"
#include "woopsistring.h"
#include "colourpicker.h"
#include "filerequester.h"
#include "soundTGDS.h"
#include "main.h"

__attribute__((section(".itcm")))
WoopsiTemplate * WoopsiTemplateProc = NULL;

void WoopsiTemplate::startup() {
	Rect rect;

	/** SuperBitmap preparation **/
	// Create bitmap for superbitmap
	Bitmap* superBitmapBitmap = new Bitmap(164, 191);

	// Get a graphics object from the bitmap so that we can modify it
	Graphics* gfx = superBitmapBitmap->newGraphics();

	// Clean up
	delete gfx;

	// Create screens
	AmigaScreen* newScreen = new AmigaScreen(TGDSPROJECTNAME, Gadget::GADGET_DRAGGABLE, AmigaScreen::AMIGA_SCREEN_SHOW_DEPTH | AmigaScreen::AMIGA_SCREEN_SHOW_FLIP);
	woopsiApplication->addGadget(newScreen);
	newScreen->setPermeable(true);

	// Add child windows
	AmigaWindow* controlWindow = new AmigaWindow(0, 13, 256, 33, "Controls", Gadget::GADGET_DRAGGABLE, AmigaWindow::AMIGA_WINDOW_SHOW_DEPTH);
	newScreen->addGadget(controlWindow);

	// Controls
	controlWindow->getClientRect(rect);

	Button* homeButton = new Button(rect.x, rect.y, 41, 16, "Home");
	homeButton->disable();

	controlWindow->addGadget(homeButton);
	controlWindow->addGadget(new Button(rect.x + 41, rect.y, 49, 16, "Index"));
	controlWindow->addGadget(new Button(rect.x + 90, rect.y, 17, 16, "<"));
	controlWindow->addGadget(new Button(rect.x + 107, rect.y, 17, 16, ">"));
	controlWindow->addGadget(new Button(rect.x + 124, rect.y, 40, 16, "Help"));

	// Add File listing screen
	AmigaScreen* fileScreen = new AmigaScreen("File List", Gadget::GADGET_DRAGGABLE, AmigaScreen::AMIGA_SCREEN_SHOW_DEPTH | AmigaScreen::AMIGA_SCREEN_SHOW_FLIP);
	woopsiApplication->addGadget(fileScreen);
	fileScreen->setPermeable(true);
	fileScreen->flipToTopScreen();
	// Add screen background
	fileScreen->insertGadget(new Gradient(0, SCREEN_TITLE_HEIGHT, 256, 192 - SCREEN_TITLE_HEIGHT, woopsiRGB(0, 31, 0), woopsiRGB(0, 0, 31)));
	
	// Create FileRequester
	fileReq = new FileRequester(10, 10, 150, 150, "Files", "/", GADGET_DRAGGABLE | GADGET_DOUBLE_CLICKABLE);
	fileReq->setRefcon(1);
	//Setting event handlers
	fileReq->addGadgetEventHandler(this);
	fileScreen->addGadget(fileReq);
	
	enableDrawing();	// Ensure Woopsi can now draw itself
	redraw();			// Draw initial state
}

void WoopsiTemplate::shutdown() {
	Woopsi::shutdown();
}

void WoopsiTemplate::handleValueChangeEvent(const GadgetEventArgs& e) {

	// Did a gadget fire this event?
	if (e.getSource() != NULL) {
	
		// Is the gadget the file requester?
		if (e.getSource()->getRefcon() == 1) {
			
			//Play WAV/ADPCM if selected from the FileRequester
			WoopsiString strObj = ((FileRequester*)e.getSource())->getSelectedOption()->getText();
			char fName[256+1];
			memset(fName, 0, sizeof(fName));
			strObj.copyToCharArray((char*)&fName[0]);
			internalCodecType = playSoundStream(fName, _FileHandleVideo, _FileHandleAudio);
			
		}
	}
}

void WoopsiTemplate::handleLidClosed() {
	// Lid has just been closed
	_lidClosed = true;

	// Run lid closed on all gadgets
	s32 i = 0;
	while (i < _gadgets.size()) {
		_gadgets[i]->lidClose();
		i++;
	}
}

void WoopsiTemplate::handleLidOpen() {
	// Lid has just been opened
	_lidClosed = false;

	// Run lid opened on all gadgets
	s32 i = 0;
	while (i < _gadgets.size()) {
		_gadgets[i]->lidOpen();
		i++;
	}
}

//Called once Woopsi events are ended: TGDS Main Loop
__attribute__((section(".itcm")))
void Woopsi::ApplicationMainLoop(){
	//Earlier.. main from Woopsi SDK.
	
	//Handle TGDS stuff...
}