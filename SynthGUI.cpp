/// GUI for synthengine process

#include <stdio.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <vector>
#include <string>
#include <math.h>
#include <assert.h>

#include "common.h"

#define DrawText DrawText		// stupid Windows
#include "fontengine.h"
#include "GUI.h"

//#define FREQ 220 /* the frequency we want */

#define WIDTH	480
#define HEIGHT	272

// For rendering text
FontEngine *bigFont = NULL;
FontEngine *smallFont = NULL;

// These must be global 
std::string wfNames[] = {
	"NONE",
	"SQUARE",
	"SAW1",
	"SAW2",
	"TRIANGLE",
	"SINE",
	"NOISE"
};

// Curent patch object
CPatch workPatch;

static void PostMessage(mqd_t mq, char *buffer, int length)
{
	mq_send(mq, buffer, length, 0);
}

/// Patch Select screen
int DoPatchSelect(SDL_Renderer *renderer, mqd_t mqEngine, mqd_t mqGUI)
{
	std::string names[32];
	
	for (int i = 0; i < 32; i++)
		{
		// Send "Request patch data" message to the synth engine
		// Use "undefined" CC message 103
		char outBuffer[MSG_MAX_SIZE] = { 0xB0, 103, (char)i };
		PostMessage(mqEngine, outBuffer, 3);

		SDL_Delay(20);
		
		// Check for data from the synthengine process
		char buffer[MSG_MAX_SIZE];
		ssize_t bytes_read = mq_receive(mqGUI, buffer, MSG_MAX_SIZE, NULL);
		if (bytes_read > 0)
			{
			buffer[bytes_read] = 0;
			//printf("msg received: %s\n", buffer);
			// All messages are MIDI messages
			char command = buffer[0];
			if (0xF0 == command)
				{
				// Sysex - Synth engine is sending patch data
				if (0x7D == buffer[1])
					{
					char sysexCmd = buffer[4];
					char paramAddr = buffer[6];		// param "address"
					char sysexSize = buffer[7];		// size of data (or data requested)
					if (0x12 == sysexCmd)				// "DT1" - recieve patch data
						{
						// read and convert the data
						char *p = buffer + 8;
						CPatch patch;
						patch.UnpackParams(p, paramAddr, sysexSize);
						names[i].assign(patch.GetName());
						//printf("%s\n", patch.GetName());
						}
					}
				}
			}
		}

	
	
	// Setup the popup layout
	int parentWidth, parentHeight;
	SDL_RenderGetLogicalSize(renderer, &parentWidth, &parentHeight);

	SDL_Rect gmRect = { 0, 0, parentWidth, parentHeight };
	CGUIManager gm(gmRect);
	gm.m_drawContext.m_renderer = renderer;
	gm.m_drawContext.m_font = smallFont;
	gm.m_drawContext.SetForeColour(255, 255, 255, 0);
	gm.m_drawContext.SetBackColour(0, 96, 0, 255);

	// Ask the synth engine for the patch names
	//RequestPatchNames(mqEngine);
	
	// Add the controls to the layout
	const int COLUMNS = 4;
	const int ROWS = 8;
	const int MARGIN = 4;
	const int CW = (gmRect.w / COLUMNS) - MARGIN;
	const int CH = (gmRect.h / ROWS) - MARGIN;
	for (int i = 0; i < 32; i++)
		{
		gm.AddControl((MARGIN + CW) * (i % 4), (MARGIN + CH) * (i/4), CW, CH, CT_BUTTON,  "PatchName", names[i].c_str());
		}

	int selectedIndex = -1;
	SDL_Event event;
	bool done = false;
	while(!done)
		{
		while (SDL_PollEvent(&event))
			{
			switch (event.type)
				{
				case SDL_KEYDOWN :
					// Check for ESC pressed
					if (SDLK_ESCAPE == event.key.keysym.sym)
						done = true;
					break;
				case SDL_MOUSEBUTTONUP :
					// Get the relevant control from the gui manager
					CGUIControl *control = gm.OnMouseUp(event.button.x, event.button.y);
					if (control)
						{
						// get "local coords"
						int x = control->m_rect.x;
						int y = control->m_rect.y;
						const int ix = x / (MARGIN + CW);
						const int iy = y / (MARGIN + CH);
						selectedIndex  = ix + COLUMNS * iy;
						done = true;
						}
					break;
				}	// end switch
			}	// wend event


		//SDL_Rect rect;
		//rect.x = 50;
		//rect.y = 50;
		//rect.w = 200;
		//rect.h = 200;
		//bigFont->DrawText(renderer, "Hello James", rect, true);
		//rect.y = 120;
		//smallFont->DrawText(renderer, "Hello James", rect, true);

		//SDL_RenderDrawLine(renderer, 10, 10, 400, 200);
		
		gm.DrawAllControls();
		
		//SDL_Flip(screen);
		SDL_RenderPresent(renderer);
		SDL_Delay(10);
		}

	printf("Selected patch %d.\n", selectedIndex);

	return selectedIndex;
}


bool reverbOn = true;

/// Set up the main synth GUI screen
static int SetupMainPage(CGUIManager &gm)
{
	// Patch name edit
	gm.AddControl(10, 10, 330, 20, CT_EDIT,    "PatchName", "_patchname");
	gm.AddControl(350, 10, 40, 20, CT_BUTTON,  "SavePatch", "Save");

	// OSC 1 parameters
	gm.AddControl(10, 40, 50, 10,   CT_LABEL,      "OSC1Label", "OSC1");
	gm.AddControl(20, 60, 70, 10,   CT_LABEL,      "WF1Label", "Waveform");
	gm.AddControl(20, 80, 70, 10,   CT_LABEL,      "Duty1Label", "Duty %");
	gm.AddControl(20, 100, 70, 10,  CT_LABEL,      "Detune1Label", "Detune");
	gm.AddControl(20, 120, 70, 10,  CT_LABEL,      "VEnv1Label", "Vol Env");
	gm.AddControl(20, 140, 70, 10,  CT_LABEL,      "PEnv1Label", "Pitch Env");
	gm.AddControl(20, 160, 70, 10,  CT_LABEL,      "FEnv1Label", "Filter Env");
	gm.AddControl(20, 180, 70, 10,  CT_LABEL,      "LFOVEnv1Label", "LFO V Env");
	gm.AddControl(20, 200, 70, 10,  CT_LABEL,      "LFOPEnv1Label", "LFO P Env");
	gm.AddControl(20, 220, 70, 10,  CT_LABEL,      "LFOFEnv1Label", "LFO F Env");

	gm.AddControl(100, 58, 60, 17,  CT_OPTIONLIST, "WF1Option", NULL);
	gm.AddControl(100, 78, 60, 17,  CT_SLIDER,     "Duty1Slider", NULL);
	gm.AddControl(100, 98, 60, 17,  CT_SLIDER,     "Detune1Slider", NULL);
	gm.AddControl(100, 118, 60, 17, CT_ENVELOPE,   "VEnv1Envelope", NULL);
	gm.AddControl(100, 138, 60, 17, CT_ENVELOPE,   "PEnv1Envelope", NULL);
	gm.AddControl(100, 158, 60, 17, CT_ENVELOPE,   "FEnv1Envelope", NULL);
	gm.AddControl(100, 178, 60, 17, CT_ENVELOPE,   "LFOVEnv1Envelope", NULL);
	gm.AddControl(100, 198, 60, 17, CT_ENVELOPE,   "LFOPEnv1Envelope", NULL);
	gm.AddControl(100, 218, 60, 17, CT_ENVELOPE,   "LFOFEnv1Envelope", NULL);


	// OSC 2 parameters
	gm.AddControl(170, 40, 50, 10,   CT_LABEL,      "OSC2Label", "OSC1");
	gm.AddControl(180, 60, 70, 10,   CT_LABEL,      "WF2Label", "Waveform");
	gm.AddControl(180, 80, 70, 10,   CT_LABEL,      "Duty2Label", "Duty %");
	gm.AddControl(180, 100, 70, 10,  CT_LABEL,      "Detune2Label", "Detune");
	gm.AddControl(180, 120, 70, 10,  CT_LABEL,      "VEnv2Label", "Vol Env");
	gm.AddControl(180, 140, 70, 10,  CT_LABEL,      "PEnv2Label", "Pitch Env");
	gm.AddControl(180, 160, 70, 10,  CT_LABEL,      "FEnv2Label", "Filter Env");
	gm.AddControl(180, 180, 70, 10,  CT_LABEL,      "LFOVEnv2Label", "LFO V Env");
	gm.AddControl(180, 200, 70, 10,  CT_LABEL,      "LFOPEnv2Label", "LFO P Env");
	gm.AddControl(180, 220, 70, 10,  CT_LABEL,      "LFOFEnv2Label", "LFO F Env");

	gm.AddControl(260, 58, 60, 17,  CT_OPTIONLIST, "WF2Option", NULL);
	gm.AddControl(260, 78, 60, 17,  CT_SLIDER,     "Duty2Slider", NULL);
	gm.AddControl(260, 98, 60, 17,  CT_SLIDER,     "Detune2Slider", NULL);
	gm.AddControl(260, 118, 60, 17, CT_ENVELOPE,   "VEnv2Envelope", NULL);
	gm.AddControl(260, 138, 60, 17, CT_ENVELOPE,   "PEnv2Envelope", NULL);
	gm.AddControl(260, 158, 60, 17, CT_ENVELOPE,   "FEnv2Envelope", NULL);
	gm.AddControl(260, 178, 60, 17, CT_ENVELOPE,   "LFOVEnv2Envelope", NULL);
	gm.AddControl(260, 198, 60, 17, CT_ENVELOPE,   "LFOPEnv2Envelope", NULL);
	gm.AddControl(260, 218, 60, 17, CT_ENVELOPE,   "LFOFEnv2Envelope", NULL);

	// LFO parameters
	gm.AddControl(330, 40, 50, 10,  CT_LABEL,      "LFOLabel", "LFO");
	gm.AddControl(340, 60, 70, 10,  CT_LABEL,      "LFOWFLabel", "Waveform");
	gm.AddControl(340, 80, 70, 10,  CT_LABEL,      "LFOFreqLabel", "Freq.");
	gm.AddControl(340, 100, 70, 10, CT_LABEL,      "LFODepthLabel", "Depth");

	gm.AddControl(420, 58, 60, 17,  CT_OPTIONLIST, "LFOWFOption", NULL);
	gm.AddControl(420, 78, 60, 17,  CT_SLIDER,     "LFOFreqSlider", NULL);
	gm.AddControl(420, 98, 60, 17,  CT_SLIDER,     "LFODepthSlider", NULL);

	// Reverb parameters
	gm.AddControl(340, 140, 50, 10, CT_LABEL,      "RvbDepthLabel", "Rev Depth");
	gm.AddControl(420, 138, 60, 17, CT_SLIDER,     "RvbDepth", NULL);

	// Filter parameters
	gm.AddControl(340, 180, 50, 10, CT_LABEL,      "FilterCutoffLabel", "Filt Cutoff");
	gm.AddControl(420, 178, 60, 17, CT_SLIDER,     "FilterCutoff", NULL);

/*

10   190  40    10   CT_LABEL		"MODLabel"	"MOD"
20   210  70    10   CT_LABEL		"PB Range"	""
20   230  70    10   CT_LABEL		"Mod Target"	""
20   250  70    10   CT_LABEL		"Mod Range"	""
100  205  60    17   CT_SLIDER		"PBRangeSlider"	""
100  225  60    17   CT_OPTIONLIST	"ModTargetOption"	""
100  245  60    17   CT_SLIDER		"ModRangeSlider"	""
*/

	gm.AddControl(10, 250, 120, 20, CT_BUTTON,  "SelectPatch", "Select Patch");
	
	CGUIOptionList *wf1OptList = (CGUIOptionList *)gm.GetControl("WF1Option");
	CGUIOptionList *wf2OptList = (CGUIOptionList *)gm.GetControl("WF2Option");
	CGUIOptionList *lfoWfOptList = (CGUIOptionList *)gm.GetControl("LFOWFOption");
	if (!wf1OptList || !wf2OptList || !lfoWfOptList)
		printf("NULL optlist!\n");

	for (int i = 0; i < 7; i++)
		{
		wf1OptList->m_optionArray.push_back(wfNames[i]);
		wf2OptList->m_optionArray.push_back(wfNames[i]);
		lfoWfOptList->m_optionArray.push_back(wfNames[i]);
		}
	wf1OptList->m_selectedIndex = 0;
	wf2OptList->m_selectedIndex = 0;
	lfoWfOptList->m_selectedIndex = 0;

	return gm.m_controls.size();
}

/// Setup ADSR envelope control from patch envelope data
static void SetEnvelopeControlData(CGUIEnvelope *control, const CEnvelope *envData)
{
	if (control && envData)
		control->SetADSR(envData->m_delay, envData->m_attack, envData->m_peak, envData->m_decay, envData->m_sustain, envData->m_release);
}

/// Set patch envelope data from ADSR envelope control
static void GetEnvelopeDataFromControl(CGUIEnvelope *control, CEnvelope *envData)
{
	if (control && envData)
		control->GetADSR(envData->m_delay, envData->m_attack, envData->m_peak, envData->m_decay, envData->m_sustain, envData->m_release);
}

/// Refresh the main page from the specified patch
static void UpdateMainPageFromPatch(CGUIManager &gm, const CPatch &patch)
{
	CGUIControl *control = gm.GetControl("PatchName");
	control->SetText(patch.GetName());
	
	// OSC1
	control = gm.GetControl("WF1Option");
	((CGUIOptionList *)control)->SetSelectedIndex(patch.m_osc1.m_waveType);

	control = gm.GetControl("Duty1Slider");
	((CGUISlider *)control)->m_value = patch.m_osc1.m_duty;
	((CGUISlider *)control)->m_minVal = 0.0f;
	((CGUISlider *)control)->m_maxVal = 1.0f;

	control = gm.GetControl("Detune1Slider");
	((CGUISlider *)control)->m_value = patch.m_osc1.m_detune;
	((CGUISlider *)control)->m_minVal = -1.0f;
	((CGUISlider *)control)->m_maxVal = 1.0f;

	control = gm.GetControl("VEnv1Envelope");
	SetEnvelopeControlData((CGUIEnvelope *)control, &patch.m_osc1.m_envelope[ET_VOLUME]);
	control = gm.GetControl("PEnv1Envelope");
	SetEnvelopeControlData((CGUIEnvelope *)control, &patch.m_osc1.m_envelope[ET_PITCH]);
	control = gm.GetControl("FEnv1Envelope");
	SetEnvelopeControlData((CGUIEnvelope *)control, &patch.m_osc1.m_envelope[ET_FILTER]);

	control = gm.GetControl("LFOVEnv1Envelope");
	SetEnvelopeControlData((CGUIEnvelope *)control, &patch.m_osc1.m_LFOEnvelope[ET_VOLUME]);
	control = gm.GetControl("LFOPEnv1Envelope");
	SetEnvelopeControlData((CGUIEnvelope *)control, &patch.m_osc1.m_LFOEnvelope[ET_PITCH]);
	control = gm.GetControl("LFOFEnv1Envelope");
	SetEnvelopeControlData((CGUIEnvelope *)control, &patch.m_osc1.m_LFOEnvelope[ET_FILTER]);

	// OSC2
	control = gm.GetControl("WF2Option");
	((CGUIOptionList *)control)->SetSelectedIndex(patch.m_osc2.m_waveType);

	control = gm.GetControl("Duty2Slider");
	((CGUISlider *)control)->m_value = patch.m_osc2.m_duty;
	((CGUISlider *)control)->m_minVal = 0.0f;
	((CGUISlider *)control)->m_maxVal = 1.0f;

	control = gm.GetControl("Detune2Slider");
	((CGUISlider *)control)->m_value = patch.m_osc2.m_detune;
	((CGUISlider *)control)->m_minVal = -1.0f;
	((CGUISlider *)control)->m_maxVal = 1.0f;

	control = gm.GetControl("VEnv2Envelope");
	SetEnvelopeControlData((CGUIEnvelope *)control, &patch.m_osc2.m_envelope[ET_VOLUME]);
	control = gm.GetControl("PEnv2Envelope");
	SetEnvelopeControlData((CGUIEnvelope *)control, &patch.m_osc2.m_envelope[ET_PITCH]);
	control = gm.GetControl("FEnv2Envelope");
	SetEnvelopeControlData((CGUIEnvelope *)control, &patch.m_osc2.m_envelope[ET_FILTER]);

	control = gm.GetControl("LFOVEnv2Envelope");
	SetEnvelopeControlData((CGUIEnvelope *)control, &patch.m_osc2.m_LFOEnvelope[ET_VOLUME]);
	control = gm.GetControl("LFOPEnv2Envelope");
	SetEnvelopeControlData((CGUIEnvelope *)control, &patch.m_osc2.m_LFOEnvelope[ET_PITCH]);
	control = gm.GetControl("LFOFEnv2Envelope");
	SetEnvelopeControlData((CGUIEnvelope *)control, &patch.m_osc2.m_LFOEnvelope[ET_FILTER]);

	// LFO
	control = gm.GetControl("LFOWFOption");
	((CGUIOptionList *)control)->SetSelectedIndex(patch.m_LFOWaveform);

	control = gm.GetControl("LFOFreqSlider");
	((CGUISlider *)control)->m_value = patch.m_LFOFreq;
	((CGUISlider *)control)->m_minVal = 0.1f;
	((CGUISlider *)control)->m_maxVal = LFO_MAX_FREQ;

	control = gm.GetControl("LFODepthSlider");
	((CGUISlider *)control)->m_value = patch.m_LFODepth;
	// default slider range is 0.0 to 1.0

}

/// Update the patch data from the controls
static void UpdatePatchFromMainPage(CGUIManager &gm, CPatch &patch)
{
	CGUIControl *control = gm.GetControl("PatchName");
	patch.SetName(control->GetText());
	
	// OSC1
	control = gm.GetControl("WF1Option");
	patch.m_osc1.m_waveType = (WAVETYPE)((CGUIOptionList *)control)->GetSelectedIndex();

	control = gm.GetControl("Duty1Slider");
	patch.m_osc1.m_duty = ((CGUISlider *)control)->m_value;

	control = gm.GetControl("Detune1Slider");
	patch.m_osc1.m_detune = ((CGUISlider *)control)->m_value;

	control = gm.GetControl("VEnv1Envelope");
	GetEnvelopeDataFromControl((CGUIEnvelope *)control, &patch.m_osc1.m_envelope[ET_VOLUME]);
	control = gm.GetControl("PEnv1Envelope");
	GetEnvelopeDataFromControl((CGUIEnvelope *)control, &patch.m_osc1.m_envelope[ET_PITCH]);
	control = gm.GetControl("FEnv1Envelope");
	GetEnvelopeDataFromControl((CGUIEnvelope *)control, &patch.m_osc1.m_envelope[ET_FILTER]);

	control = gm.GetControl("LFOVEnv1Envelope");
	GetEnvelopeDataFromControl((CGUIEnvelope *)control, &patch.m_osc1.m_LFOEnvelope[ET_VOLUME]);
	control = gm.GetControl("LFOPEnv1Envelope");
	GetEnvelopeDataFromControl((CGUIEnvelope *)control, &patch.m_osc1.m_LFOEnvelope[ET_PITCH]);
	control = gm.GetControl("LFOFEnv1Envelope");
	GetEnvelopeDataFromControl((CGUIEnvelope *)control, &patch.m_osc1.m_LFOEnvelope[ET_FILTER]);

	// OSC2
	control = gm.GetControl("WF2Option");
	patch.m_osc2.m_waveType = (WAVETYPE)((CGUIOptionList *)control)->GetSelectedIndex();

	control = gm.GetControl("Duty2Slider");
	patch.m_osc2.m_duty = ((CGUISlider *)control)->m_value;

	control = gm.GetControl("Detune2Slider");
	patch.m_osc2.m_detune = ((CGUISlider *)control)->m_value;

	control = gm.GetControl("VEnv2Envelope");
	GetEnvelopeDataFromControl((CGUIEnvelope *)control, &patch.m_osc2.m_envelope[ET_VOLUME]);
	control = gm.GetControl("PEnv2Envelope");
	GetEnvelopeDataFromControl((CGUIEnvelope *)control, &patch.m_osc2.m_envelope[ET_PITCH]);
	control = gm.GetControl("FEnv2Envelope");
	GetEnvelopeDataFromControl((CGUIEnvelope *)control, &patch.m_osc2.m_envelope[ET_FILTER]);

	control = gm.GetControl("LFOVEnv2Envelope");
	GetEnvelopeDataFromControl((CGUIEnvelope *)control, &patch.m_osc2.m_LFOEnvelope[ET_VOLUME]);
	control = gm.GetControl("LFOPEnv2Envelope");
	GetEnvelopeDataFromControl((CGUIEnvelope *)control, &patch.m_osc2.m_LFOEnvelope[ET_PITCH]);
	control = gm.GetControl("LFOFEnv2Envelope");
	GetEnvelopeDataFromControl((CGUIEnvelope *)control, &patch.m_osc2.m_LFOEnvelope[ET_FILTER]);
	
	// LFO
	control = gm.GetControl("LFOWFOption");
	patch.m_LFOWaveform = (WAVETYPE)((CGUIOptionList *)control)->GetSelectedIndex();

	control = gm.GetControl("LFOFreqSlider");
	patch.m_LFOFreq = ((CGUISlider *)control)->m_value;
	
	control = gm.GetControl("LFODepthSlider");
	patch.m_LFODepth = ((CGUISlider *)control)->m_value;
}

/// Process messages recieved from the synth engine
static void ProcessMessage(char *buffer, int length)
{
	// parse buffer and do something (see ref WaveSynth::ProcessMessage())
	// TODO : Handle patch info recieve
	if (length < 1)
		return;
		
	// All messages are MIDI messages
	char command = buffer[0];
	switch (command & 0xF0)
		{
		case 0xB0 :			// Controller change
			{
			//char cc = buffer[1];
			// TODO
			}
			break;
		case 0xC0 :			// Patch change
			{
			//char patchIndex = buffer[1];
			//m_currentPatch = patchIndex;
			}
			break;
		case 0xE0 :			// Pitch bend
			{
			//char bend = buffer[1];
			}
			break;
		case 0XF0 :			// System / sysex
			{
			if (0xF0 == command)
				{
				// Sysex - Synth engine is sending patch data
				if (0x7D == buffer[1])
					{
					char sysexCmd = buffer[4];
					char paramAddr = buffer[6];		// param "address"
					char sysexSize = buffer[7];		// size of data (or data requested)
					if (0x12 == sysexCmd)				// "DT1" - recieve patch data
						{
						// read and convert the data
						char *p = buffer + 8; 
						workPatch.UnpackParams(p, paramAddr, sysexSize);
						printf("Recieved patch '%s' data.\n", workPatch.GetName()); 
						}
					}
				}
			}
			break;
		}	// end switch
}

/// Request current (working) patch data from the synth engine
static void RequestPatchData(mqd_t mq)
{
	// create MIDI sysex RQ1 message to request the patch data
	char buffer[] = { 0xF0, 0x7D, 0x01, 0x01, 0x11, 0x00, 0x00, PADDR_END, 0x00, 0xF7 } ;
	PostMessage(mq, buffer, 10);
	
	// Synth engine will send DT1 sysex data
}

int main(int argc, char *argv[])
{
	// IPC message queue stuff
	char mqbuffer[MSG_MAX_SIZE];
	
	// Open queue for sending messages to the synth engine
	mqd_t mqEngine = mq_open(ENGINE_QUEUE_NAME, O_WRONLY | O_NONBLOCK);
	//assert(mqEngine != (mqd_t)-1);
	if (mqEngine == (mqd_t)-1)
		return -1;

	// Open queue for reading message from the synth engine
	mqd_t mqGUI = mq_open(GUI_QUEUE_NAME, O_RDONLY | O_NONBLOCK);
	//assert(mqGUI != (mqd_t)-1);
	if (mqEngine == (mqd_t)-1)
		return -1;


 	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);

	//SDL_Surface *screen = SDL_SetVideoMode(WIDTH, HEIGHT, 0, 0);
	SDL_Window *window = SDL_CreateWindow("SynthGUI", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	if (!window)
		printf("SDL_CreateWindow failed.\n");
		
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	if (!renderer)
		printf("SDL_CreateRenderer failed.\n");

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	bigFont = new FontEngine(renderer, "font_16x32.bmp", 16, 32);
	smallFont = new FontEngine(renderer, "font_8x16.bmp", 8, 16);

	// Create the GUI control manager and set up our layout
	int guiWidth, guiHeight;
	SDL_GetWindowSize(window, &guiWidth, &guiHeight);
	// This needed so controls can determine parent window size
	SDL_RenderSetLogicalSize(renderer, guiWidth, guiHeight);

	SDL_Rect gmRect = { 0, 0, guiWidth, guiHeight };
	CGUIManager gm(gmRect);
	gm.m_drawContext.m_renderer = renderer;
	gm.m_drawContext.m_font = smallFont;
	gm.m_drawContext.SetForeColour(255, 255, 128, 0);
	gm.m_drawContext.SetBackColour(0, 0, 0, 255);
	SetupMainPage(gm);

	// Ask the synth engine for current patch info
	RequestPatchData(mqEngine);

	SDL_Event event;
	bool done = false;
	while(!done)
		{
		while (SDL_PollEvent(&event))
			{
			switch (event.type)
				{
				case SDL_QUIT :
					done = true;
					break;
				case SDL_KEYDOWN :
					// Trigger note from keyboard
					mqbuffer[0] = 0x80;
					mqbuffer[1] = (char)event.key.keysym.sym;
					mqbuffer[2] = 0x7F;
					mqbuffer[4] = 0;
					printf("Posting NoteOn message...\n");
					PostMessage(mqEngine, mqbuffer, 4);
					break;
				case SDL_KEYUP :
					// Note up
					mqbuffer[0] = 0x90;
					mqbuffer[1] = (char)event.key.keysym.sym;
					mqbuffer[2] = 0;
					printf("Posting NoteOff message...\n");
					PostMessage(mqEngine, mqbuffer, 3);
					break;
				case SDL_JOYBUTTONDOWN :
					break;
				case SDL_JOYBUTTONUP :
					break;
				case SDL_MOUSEMOTION :
					{
					// Trap dragging of sliders etc
					// Let gui manager handle the event first
					CGUIControl *control = gm.OnMouseMove(event.button.x, event.button.y);
					if (control)
						{
						if (0 == strcmp(control->m_name, "FilterCutoff"))
							{
							// Use "filter cutoff" CC message 74
							unsigned char value = (unsigned char)(((CGUISlider *)control)->m_value * 0x7F);
							char outBuffer[MSG_MAX_SIZE] = { 0xB0, 74, value };
							PostMessage(mqEngine, outBuffer, 3);
							gm.m_controlChanged = false;
							}
						else if (0 == strcmp(control->m_name, "RvbDepth"))
							{
							// Use "Reverb Send" CC message 91
							unsigned char value = (unsigned char)(((CGUISlider *)control)->m_value * 0x7F);
							char outBuffer[MSG_MAX_SIZE] = { 0xB0, 91, value };
							PostMessage(mqEngine, outBuffer, 3);
							gm.m_controlChanged = false;
							}
						}
					}
					break;
				case SDL_MOUSEBUTTONDOWN :
					gm.OnMouseDown(event.button.x, event.button.y);
					break;
				case SDL_MOUSEBUTTONUP :
					{
					// Let the gui manager handle the mouse up first
					CGUIControl *control = gm.OnMouseUp(event.button.x, event.button.y);
					// Now we see if we need to handle it
					//HandleControlClick(control, mqEngine);
					if (control)
						{
						printf("Button clicked: %s\n", control->m_name);
						if (0 == strcmp(control->m_name, "SavePatch"))
							{
							// Send "Save" message to the synth engine
							// Use "undefined" CC message 102
							char outBuffer[MSG_MAX_SIZE] = { 0xB0, 102, 1 };
							PostMessage(mqEngine, outBuffer, 3);
							gm.m_controlChanged = false;
							}
						else if (0 == strcmp(control->m_name, "FilterCutoff"))
							{
							// Use "filter cutoff" CC message 74
							unsigned char value = (unsigned char)(((CGUISlider *)control)->m_value * 0x7F);
							char outBuffer[MSG_MAX_SIZE] = { 0xB0, 74, value };
							PostMessage(mqEngine, outBuffer, 3);
							gm.m_controlChanged = false;
							}
						else if (0 == strcmp(control->m_name, "SelectPatch"))
							{
							// Show Patch Select screen
							int selectedPatch = DoPatchSelect(renderer, mqEngine, mqGUI);
							// Send patch select message to the engine
							mqbuffer[0] = 0xC0;
							mqbuffer[1] = (char)selectedPatch;
							mqbuffer[2] = 0;
							printf("Posting Patch %d Select message...\n", selectedPatch);
							PostMessage(mqEngine, mqbuffer, 3);
							// Ask the synth engine for current patch info
							RequestPatchData(mqEngine);
							// Avoid sending patch data to the engine before we get it back!
							gm.m_controlChanged = false;
							}
						else if (0 == strcmp(control->m_name, "RvbDepth"))
							{
							// Use "Reverb Send" CC message 91
							unsigned char value = (unsigned char)(((CGUISlider *)control)->m_value * 0x7F);
							char outBuffer[MSG_MAX_SIZE] = { 0xB0, 91, value };
							PostMessage(mqEngine, outBuffer, 3);
							gm.m_controlChanged = false;
							}
						}
					}
					break;
				}	// end switch
			}	// wend event

		// If a control has changed, then send patch data to synth
		if (gm.m_controlChanged)
			{
			printf("Control changed!\n");
			UpdatePatchFromMainPage(gm, workPatch);
			// Use Patch::PackParams() to create data for message
			//        to send
			char outBuffer[MSG_MAX_SIZE] = {
				0xF0, 0x7D, 0x01, 0x01, 0x12, 0x00, 0x00, PADDR_END, 0x00, 0xF7
				} ;
			char *p = outBuffer + 8; 
			int packedBytes = workPatch.PackParams(p);
			PostMessage(mqEngine, outBuffer, packedBytes + 10);
			// reset changed flag
			gm.m_controlChanged = false;
			}
			
		// Check for data from the synthengine process
		ssize_t bytes_read = mq_receive(mqGUI, mqbuffer, MSG_MAX_SIZE, NULL);
		if (bytes_read > 0)
			{
			mqbuffer[bytes_read] = 0;
			//printf("msg received: %s\n", mqbuffer);
			ProcessMessage(mqbuffer, bytes_read);
			UpdateMainPageFromPatch(gm, workPatch);
			}

		//SDL_Rect rect;
		//rect.x = 50;
		//rect.y = 50;
		//rect.w = 200;
		//rect.h = 200;
		//bigFont->DrawText(renderer, "Hello James", rect, true);
		//rect.y = 120;
		//smallFont->DrawText(renderer, "Hello James", rect, true);

		//SDL_RenderDrawLine(renderer, 10, 10, 400, 200);
		
		gm.DrawAllControls();
		
		//SDL_Flip(screen);
		SDL_RenderPresent(renderer);
		SDL_Delay(10);
		}

	printf("GUI finished.\n");
	
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	// Tidy up mqueue stuff
	mq_close(mqEngine);
	mq_close(mqGUI);
	//mq_unlink(MQUEUE_NAME);    // only server (creator) has to do this

	return 0;
}
