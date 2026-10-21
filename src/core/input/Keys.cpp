// Keys.cpp - names of the keys.

#include "input/Keys.hpp"

#include <string_view>

namespace ljn {
namespace {

//keys that have a fixed name, sorted by code
struct NamedKey {
    int code;
    std::string_view name;
};

constexpr NamedKey kNamedKeys[] = {
    {32, "Space"},          {39, "'"},           {44, ","},           {45, "-"},          {46, "."},
    {47, "/"},              {59, ";"},           {61, "="},           {91, "["},          {92, "\\"},
    {93, "]"},              {96, "`"},           {256, "Escape"},     {257, "Enter"},     {258, "Tab"},
    {259, "Backspace"},     {260, "Insert"},     {261, "Delete"},     {262, "Right"},     {263, "Left"},
    {264, "Down"},          {265, "Up"},         {266, "Page Up"},    {267, "Page Down"}, {268, "Home"},
    {269, "End"},           {280, "Caps Lock"},  {281, "Scroll Lock"}, {282, "Num Lock"}, {283, "Print Screen"},
    {284, "Pause"},         {330, "Num ."},      {331, "Num /"},      {332, "Num *"},     {333, "Num -"},
    {334, "Num +"},         {335, "Num Enter"},  {336, "Num ="},      {340, "Left Shift"}, {341, "Left Ctrl"},
    {342, "Left Alt"},      {343, "Left Super"}, {344, "Right Shift"}, {345, "Right Ctrl"}, {346, "Right Alt"},
    {347, "Right Super"},   {348, "Menu"},
};

}  //anonymous namespace

std::string keyName(int key) {
    if (key == key::None) {
        return "-";
    }

    //letters and digits: the code is the character
    if (key >= 'A' && key <= 'Z') {
        return std::string(1, static_cast<char>(key));
    }
    if (key >= '0' && key <= '9') {
        return std::string(1, static_cast<char>(key));
    }

    //function keys: F1 is 290
    if (key >= key::F1 && key <= key::F12) {
        return "F" + std::to_string(key - key::F1 + 1);
    }

    //number pad digits: 0 is 320
    if (key >= 320 && key <= 329) {
        return "Num " + std::to_string(key - 320);
    }

    for (const NamedKey& named : kNamedKeys) {
        if (named.code == key) {
            return std::string(named.name);
        }
    }
    return "Key " + std::to_string(key);
}

bool isReservedKey(int key) noexcept {
    return key == key::Escape || key == key::Enter || (key >= key::F1 && key <= key::F12);
}

}  //namespace ljn
