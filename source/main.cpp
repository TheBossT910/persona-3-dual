#include <nds.h>
#include <stdio.h>

// states
#include "core/View.h"
#include "views/IntroView.h"
#include "views/MainMenuView.h"
#include "views/IwatodaiDormView.h"

volatile int frame = 0;
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
	
int main(void) {
	irqSet(IRQ_VBLANK, Vblank);

	// debug init
	// NOTE: for some reason, we cant use vram bank C. It might be because of consoleDemoInit...
	consoleDemoInit();

    // start with IntroView
    // SwitchView(new IntroView());

    // DEBUG
    SwitchView(new IwatodaiDormView());

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
