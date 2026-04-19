#include <nds.h>
#include <stdio.h>
#include <filesystem.h>

// states
#include "core/View.h"
#include "views/IntroView.h"
#include "views/MainMenuView.h"
#include "views/IwatodaiDormView.h"

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

    // start with IntroView
    // SwitchView(new IntroView());

    // DEBUG
    SwitchView(new IntroView());

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
            }
        }

		bgUpdate();
		oamUpdate(&oamMain);
	}

	return 0;
}
