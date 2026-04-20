#include <nds.h>
#include <stdio.h>
#include "core/globals.h"
#include "IntroVideoView.h"


void IntroVideoView::Init() {
    videoCtrl.init("intro", 30.0f, ViewState::INTRO, true);
}

ViewState IntroVideoView::Update() {
    return videoCtrl.update(); 
}

void IntroVideoView::Cleanup() {
    videoCtrl.cleanup();
}