#pragma once

ImVec2 operator+(const ImVec2 &lhs, const ImVec2 &rhs) {
  return {lhs.x + rhs.x, lhs.y + rhs.y};
}

ImVec2 operator+=(const ImVec2 &lhs, const ImVec2 &rhs) {
  return {lhs.x + rhs.x, lhs.y + rhs.y};
}

ImVec2 operator*=(const ImVec2 &lhs, const float &rhs) {
  return {lhs.x + rhs, lhs.y + rhs};
}

ImVec2 operator-(const ImVec2 &lhs, const ImVec2 &rhs) {
  return {lhs.x - rhs.x, lhs.y - rhs.y};
}

ImVec2 operator*(const ImVec2 &lhs, const float &rhs) {
  return {lhs.x * rhs, lhs.y * rhs};
}
