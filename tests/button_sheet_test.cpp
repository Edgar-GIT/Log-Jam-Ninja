// button_sheet_test.cpp - checks the sprite sheet maths: frame cells, the animation and the position of the buttons.

#include <print>
#include "check.hpp"
#include "ui/ButtonSheet.hpp"

using namespace ljn;

//the cells of the sheet are cut in equal parts, row by row and column by column
static void testFrameSource() {
    const SheetLayout layout;
    CHECK(layout.columns() == 4);

    //a 640x360 sheet: 4 columns of 160 and 5 rows of 72
    const PixelRect first = frameSource(640, 360, layout, 0, 0);
    CHECK(first.x == 0.0f && first.y == 0.0f && first.width == 160.0f && first.height == 72.0f);

    //row 2 (the third button), column 3 (the second hover frame)
    const PixelRect cell = frameSource(640, 360, layout, 2, 3);
    CHECK(cell.x == 480.0f && cell.y == 144.0f && cell.width == 160.0f && cell.height == 72.0f);

    //a size that does not divide evenly: the leftover pixels are ignored (1672 / 4 = 418, 941 / 5 = 188)
    const PixelRect odd = frameSource(1672, 941, layout, 4, 3);
    CHECK(odd.width == 418.0f && odd.height == 188.0f);
    CHECK(odd.x + odd.width <= 1672.0f && odd.y + odd.height <= 941.0f);
}

//idle frames are columns 0 and 1, hover frames are columns 2 and 3, and they repeat
static void testFrameColumn() {
    const SheetLayout layout;
    const SheetAnimation animation{.idleFrameSeconds = 0.5, .hoverFrameSeconds = 0.25};

    CHECK(frameColumn(layout, animation, ButtonLook::Idle, 0.0) == 0);
    CHECK(frameColumn(layout, animation, ButtonLook::Idle, 0.49) == 0);
    CHECK(frameColumn(layout, animation, ButtonLook::Idle, 0.5) == 1);
    CHECK(frameColumn(layout, animation, ButtonLook::Idle, 1.0) == 0);

    CHECK(frameColumn(layout, animation, ButtonLook::Hover, 0.0) == 2);
    CHECK(frameColumn(layout, animation, ButtonLook::Hover, 0.25) == 3);
    CHECK(frameColumn(layout, animation, ButtonLook::Hover, 0.5) == 2);

    //a sheet with one idle frame and three hover frames
    const SheetLayout other{.rows = 5, .idleFrames = 1, .hoverFrames = 3};
    CHECK(frameColumn(other, animation, ButtonLook::Idle, 7.0) == 0);
    CHECK(frameColumn(other, animation, ButtonLook::Hover, 0.5) == 3);

    //never outside the sheet, even for a time that is negative or huge
    for (const double t : {-3.3, 0.0, 12345.6}) {
        for (const ButtonLook look : {ButtonLook::Idle, ButtonLook::Hover}) {
            const int column = frameColumn(layout, animation, look, t);
            CHECK(column >= 0 && column < layout.columns());
        }
    }
}

//the areas of the buttons touch each other and the mouse finds the right one
static void testStack() {
    const ButtonStack stack{.centerX = 480.0f, .top = 300.0f, .pitch = 48.0f, .hitWidth = 170.0f};

    const PixelRect second = stack.slot(1, 5);
    CHECK(second.x == 395.0f && second.y == 348.0f && second.width == 170.0f && second.height == 48.0f);
    CHECK(stack.centerY(0, 5) == 324.0f);
    CHECK(stack.centerY(1, 5) == 372.0f);

    CHECK(stack.indexAt(480.0f, 300.0f, 5) == 0);
    CHECK(stack.indexAt(480.0f, 347.9f, 5) == 0);
    CHECK(stack.indexAt(480.0f, 348.0f, 5) == 1);
    CHECK(stack.indexAt(396.0f, 539.0f, 5) == 4);

    //above the first, below the last, left or right of the column
    CHECK(stack.indexAt(480.0f, 299.0f, 5) == -1);
    CHECK(stack.indexAt(480.0f, 540.0f, 5) == -1);
    CHECK(stack.indexAt(394.0f, 320.0f, 5) == -1);
    CHECK(stack.indexAt(565.0f, 320.0f, 5) == -1);

    //fewer items than slots
    CHECK(stack.indexAt(480.0f, 400.0f, 2) == -1);
}

//the rows can have more than one button side by side: the first row one, the others two
static void testRows() {
    const ButtonStack stack{.centerX = 480.0f, .top = 277.0f, .pitch = 84.0f, .hitWidth = 175.0f, .gap = 24.0f,
                            .rows = {1, 2, 2}};

    //the first row has one button, in the middle
    CHECK(stack.placeOf(0, 5).row == 0);
    CHECK(stack.placeOf(0, 5).column == 0);
    CHECK(stack.placeOf(0, 5).count == 1);
    CHECK(stack.centerXOf(0, 5) == 480.0f);
    CHECK(stack.centerY(0, 5) == 319.0f);

    //the second row has two, one on each side
    CHECK(stack.placeOf(1, 5).row == 1);
    CHECK(stack.placeOf(1, 5).column == 0);
    CHECK(stack.placeOf(2, 5).column == 1);
    CHECK(stack.placeOf(1, 5).count == 2);
    CHECK(stack.centerXOf(1, 5) == 380.5f);
    CHECK(stack.centerXOf(2, 5) == 579.5f);
    CHECK(stack.centerY(1, 5) == 403.0f);
    CHECK(stack.centerY(2, 5) == 403.0f);

    //the third row is like the second
    CHECK(stack.placeOf(3, 5).row == 2);
    CHECK(stack.placeOf(4, 5).row == 2);
    CHECK(stack.centerY(3, 5) == 487.0f);
    CHECK(stack.centerY(4, 5) == 487.0f);

    //the mouse finds the button of its own row
    CHECK(stack.indexAt(380.5f, 403.0f, 5) == 1);
    CHECK(stack.indexAt(579.5f, 403.0f, 5) == 2);
    CHECK(stack.indexAt(480.0f, 319.0f, 5) == 0);
    CHECK(stack.indexAt(480.0f, 487.0f, 5) == -1);  //between the two of the last row
    CHECK(stack.indexAt(280.0f, 403.0f, 5) == -1);  //left of the row
    CHECK(stack.indexAt(680.0f, 403.0f, 5) == -1);  //right of the row

    //the areas of the two buttons of a row do not touch each other
    CHECK(stack.slot(1, 5).x + stack.slot(1, 5).width < stack.slot(2, 5).x);

    //with one button less the last row has only one
    CHECK(stack.placeOf(3, 4).row == 2);
    CHECK(stack.placeOf(3, 4).count == 1);
    CHECK(stack.centerXOf(3, 4) == 480.0f);
}

int main() {
    testFrameSource();
    testFrameColumn();
    testStack();
    testRows();
    std::println("all button sheet tests passed");
}
