// Keys.hpp - key codes, the same numbers raylib uses, so the core does not need raylib.

#pragma once

#include <string>

namespace ljn::key {

//0 means no key
inline constexpr int None = 0;

inline constexpr int Space = 32;
inline constexpr int Zero = 48;
inline constexpr int Nine = 57;
inline constexpr int A = 65;
inline constexpr int B = 66;
inline constexpr int C = 67;
inline constexpr int D = 68;
inline constexpr int E = 69;
inline constexpr int F = 70;
inline constexpr int G = 71;
inline constexpr int H = 72;
inline constexpr int I = 73;
inline constexpr int J = 74;
inline constexpr int K = 75;
inline constexpr int L = 76;
inline constexpr int M = 77;
inline constexpr int N = 78;
inline constexpr int O = 79;
inline constexpr int P = 80;
inline constexpr int Q = 81;
inline constexpr int R = 82;
inline constexpr int S = 83;
inline constexpr int T = 84;
inline constexpr int U = 85;
inline constexpr int V = 86;
inline constexpr int W = 87;
inline constexpr int X = 88;
inline constexpr int Y = 89;
inline constexpr int Z = 90;
inline constexpr int Escape = 256;
inline constexpr int Enter = 257;
inline constexpr int Tab = 258;
inline constexpr int Backspace = 259;
inline constexpr int Delete = 261;
inline constexpr int Right = 262;
inline constexpr int Left = 263;
inline constexpr int Down = 264;
inline constexpr int Up = 265;
inline constexpr int F1 = 290;
inline constexpr int F12 = 301;

}  //namespace ljn::key

namespace ljn {

//a readable name for a key ("A", "Space", "Left Shift"), "Key 123" for a key without a name, "-" for no key
[[nodiscard]] std::string keyName(int key);

//keys that cannot be used for an action: escape (cancels), enter and the function keys (the game uses them)
[[nodiscard]] bool isReservedKey(int key) noexcept;

}  //namespace ljn
