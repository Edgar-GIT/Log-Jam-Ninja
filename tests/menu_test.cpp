// menu_test.cpp - checks the menu logic: the cursor, the items and what the keys and the mouse do.

#include <print>
#include "check.hpp"
#include "ui/Menu.hpp"

using namespace ljn;

namespace {

//the keys of one frame, all false
MenuInput keys() {
    return {};
}

//a menu with a title, two buttons and a toggle
Menu makeMenu() {
    Menu menu;
    menu.add(Menu::label("Options"));
    menu.add(Menu::button(1, "Play"));
    menu.add(Menu::button(2, "Quit"));
    menu.add(Menu::toggle(3, "Sound", true));
    return menu;
}

//the cursor skips the labels
static void testCursorSkipsLabels() {
    Menu menu = makeMenu();

    //the first item is a label, so the cursor is on the first button
    CHECK(menu.cursor() == 1);
    CHECK(menu.selected() != nullptr && menu.selected()->id == 1);
}

//up and down move the cursor and wrap around
static void testCursorMovement() {
    Menu menu = makeMenu();

    menu.update({.down = true});
    CHECK(menu.selected()->id == 2);
    menu.update({.down = true});
    CHECK(menu.selected()->id == 3);

    //after the last item it goes back to the first one that can be selected
    menu.update({.down = true});
    CHECK(menu.selected()->id == 1);
    menu.update({.up = true});
    CHECK(menu.selected()->id == 3);
}

//a button is used when it is confirmed
static void testButton() {
    Menu menu = makeMenu();

    const MenuEvent event = menu.update({.confirm = true});
    CHECK(event.kind == EventKind::Activated);
    CHECK(event.id == 1);

    //moving to the other button and confirming gives that one
    menu.update({.down = true});
    const MenuEvent second = menu.update({.confirm = true});
    CHECK(second.kind == EventKind::Activated);
    CHECK(second.id == 2);
}

//a toggle changes when it is confirmed, and left and right set it directly
static void testToggle() {
    Menu menu = makeMenu();
    menu.setCursor(3);

    const MenuEvent event = menu.update({.confirm = true});
    CHECK(event.kind == EventKind::Changed);
    CHECK(menu.find(3)->value == 0);

    menu.update({.right = true});
    CHECK(menu.find(3)->value == 1);
    menu.update({.left = true});
    CHECK(menu.find(3)->value == 0);
}

//a slider goes up and down in steps and never leaves its limits
static void testSlider() {
    Menu menu;
    menu.add(Menu::slider(1, "Volume", 5, 0, 10, 5));

    CHECK(menu.find(1)->value == 5);
    menu.update({.right = true});
    CHECK(menu.find(1)->value == 10);

    //at the top it stays there
    menu.update({.right = true});
    CHECK(menu.find(1)->value == 10);

    menu.update({.left = true});
    CHECK(menu.find(1)->value == 5);
    menu.update({.left = true});
    CHECK(menu.find(1)->value == 0);
    menu.update({.left = true});
    CHECK(menu.find(1)->value == 0);
}

//a choice shows one of its texts and the cursor wraps around
static void testChoice() {
    Menu menu;
    menu.add(Menu::choice(1, "Size", {"Small", "Big"}, 0));

    CHECK(menu.find(1)->value == 0);
    menu.update({.right = true});
    CHECK(menu.find(1)->value == 1);
    menu.update({.right = true});
    CHECK(menu.find(1)->value == 0);
    menu.update({.left = true});
    CHECK(menu.find(1)->value == 1);
}

//the back key is reported and nothing else happens
static void testBack() {
    Menu menu = makeMenu();

    const MenuEvent event = menu.update({.back = true});
    CHECK(event.kind == EventKind::Back);
    CHECK(event.id == -1);
}

//a disabled item does nothing
static void testDisabled() {
    Menu menu;
    MenuItem item = Menu::button(1, "Locked");
    item.enabled = false;
    menu.add(item);

    const MenuEvent event = menu.update({.confirm = true});
    CHECK(event.kind == EventKind::None);
}

//the mouse selects an item and clicking uses it
static void testMouse() {
    Menu menu = makeMenu();

    menu.hover(2);
    CHECK(menu.cursor() == 2);

    const MenuEvent event = menu.click(2);
    CHECK(event.kind == EventKind::Activated);
    CHECK(event.id == 2);
}

//clicking a slider puts the value where the click was
static void testSliderClick() {
    Menu menu;
    menu.add(Menu::slider(1, "Volume", 0, 0, 100, 1));

    menu.click(0, 0.5f);
    CHECK(menu.find(1)->value == 50);

    menu.click(0, 0.25f);
    CHECK(menu.find(1)->value == 25);

    //the ends of the slider
    menu.click(0, 0.0f);
    CHECK(menu.find(1)->value == 0);
    menu.click(0, 1.0f);
    CHECK(menu.find(1)->value == 100);
}

//items can be found and changed by id
static void testFind() {
    Menu menu = makeMenu();

    CHECK(menu.find(1) != nullptr);
    CHECK(menu.find(1)->label == "Play");
    CHECK(menu.find(99) == nullptr);

    menu.find(1)->label = "Start";
    CHECK(menu.find(1)->label == "Start");
}

//an empty menu is safe to use
static void testEmpty() {
    Menu menu;

    CHECK(menu.size() == 0);
    CHECK(menu.selected() == nullptr);
    CHECK(menu.update(keys()).kind == EventKind::None);
    CHECK(menu.click(0).kind == EventKind::None);
}

}  //anonymous namespace

//runs every test, any failed CHECK ends the program with exit code 1
int main() {
    testCursorSkipsLabels();
    testCursorMovement();
    testButton();
    testToggle();
    testSlider();
    testChoice();
    testBack();
    testDisabled();
    testMouse();
    testSliderClick();
    testFind();
    testEmpty();
    std::println("all menu tests passed");
}
