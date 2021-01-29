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
	FileRequester* fileReq;
	TextBox* _textbox;
private:
	Alert* _alert;
};
#endif

#endif
