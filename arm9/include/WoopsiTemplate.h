#ifndef _DEMO_H_
#define _DEMO_H_

#ifdef __cplusplus
#include "alert.h"
#include "woopsi.h"
#include "woopsiheaders.h"
#include "filerequester.h"
#include "textbox.h"
#include "soundTGDS.h"

#include <string>
using namespace std;

using namespace WoopsiUI;

#define TGDSPROJECTNAME ((char*) "ToolchainGenericDS-template")

class WoopsiTemplate : public Woopsi, public GadgetEventHandler {
public:
	void startup();
	void shutdown();
	void handleValueChangeEvent(const GadgetEventArgs& e);	//Handles UI events if they change
	void handleLidClosed();
	void handleLidOpen();
	void ApplicationMainLoop();
	FileRequester* fileReq;
	TextBox* _textbox;
private:
	Alert* _alert;
};
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

extern WoopsiTemplate * WoopsiTemplateProc;

#ifdef __cplusplus
}
#endif
