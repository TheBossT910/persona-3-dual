#include <nds.h>
#include <stdio.h>
#include <filesystem.h>
#include <maxmod9.h>

// states
#include "core/View.h"
#include "views/DisclaimerView.h"
#include "views/IntroView.h"
#include "views/MainMenuView.h"
#include "views/IwatodaiDormView.h"
#include "views/VideoView.h"

// controllers
#include "controllers/MusicController.h"

volatile int frame = 0;
MusicController musicCtrl;
View* currentView = nullptr;

void SwitchView(View* newView) {
    // cleanup any existing view
    if (currentView != nullptr) {
        currentView->Cleanup();

        // free memory
        delete currentView;
    }

    // load new view
    currentView = newView;
    if (currentView != nullptr) {
        currentView->Init();
    }
}

// fn for the interrupt
void Vblank() {
	frame++;
}
	
int main(int argc, char *argv[]) {
	irqSet(IRQ_VBLANK, Vblank);

    // load NitroFS
    if (!nitroFSInit(NULL)) {
        // debug init
        consoleDemoInit();
        iprintf("NitroFS Failed! ARGV is broken.\n");
    }

    // Initialize Maxmod ONCE here
    mm_ds_system sys;
    sys.mod_count  = 0;
    sys.samp_count = 0;
    sys.mem_bank   = 0;
    mmInit(&sys);

    // start with IntroView
    // SwitchView(new IntroView());

    // DEBUG
    SwitchView(new DisclaimerView());

	while(pmMainLoop()) {
		swiWaitForVBlank();

        // check state of currentView
        if (currentView != nullptr) {
            ViewState nextState = currentView->Update();
            if (nextState == ViewState::INTRO) {
                SwitchView(new IntroView());
            } else if (nextState == ViewState::MAIN_MENU) {
                SwitchView(new MainMenuView());
            } else if (nextState == ViewState::IWATODAI_DORM) {
                SwitchView(new IwatodaiDormView());
            } else if (nextState == ViewState::DISCLAIMER) {
                SwitchView(new DisclaimerView());
            } else if (nextState == ViewState::VIDEO) {
                SwitchView(new VideoView());
            }
        }

		bgUpdate();
		oamUpdate(&oamMain);
	}

	return 0;
}
