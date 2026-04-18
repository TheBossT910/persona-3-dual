#pragma once

// enum to tell which ViewState to use
enum class ViewState {
    KEEP_CURRENT,
    INTRO,
    MAIN_MENU,
    IWATODAI_DORM
};

class View {
    public:
        // the destructor (runs when we delete a view)
        // viritual is used for overriding functions (polymorphism) 
        // in this case, it allows child classes instances (ex. IntroView) to be properly cleaned up when referred throguh the parent (View) class
        virtual ~View() = default;

        // abstract methods, as marked by "= 0"
        virtual void Init() = 0;        // view setup
        virtual ViewState Update() = 0; // view update
        virtual void Cleanup() = 0;     // view cleanup
};