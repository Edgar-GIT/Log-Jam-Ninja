// Style.hpp - the colors and the text sizes of the game, all in one place.

#pragma once

#include <raylib.h>

namespace ljn::style {

//the background of the menus
inline constexpr Color kBackground{18, 20, 28, 255};

//the panels and their lines
inline constexpr Color kPanel{28, 32, 44, 255};
inline constexpr Color kPanelLine{64, 74, 98, 255};

//the text: normal, dim (things that cannot be used) and the highlight of the item under the cursor
inline constexpr Color kText{232, 236, 245, 255};
inline constexpr Color kTextDim{140, 148, 168, 255};
inline constexpr Color kAccent{120, 210, 255, 255};

//the good and the bad news
inline constexpr Color kGood{120, 220, 140, 255};
inline constexpr Color kBad{255, 120, 120, 255};

//the rectangle behind the item under the cursor
inline constexpr Color kHighlight{255, 255, 255, 38};

//the sizes of the text
inline constexpr int kTitleSize{46};
inline constexpr int kHeadingSize{30};
inline constexpr int kItemSize{22};
inline constexpr int kSmallSize{16};
inline constexpr int kHugeSize{54};

}  //namespace ljn::style
