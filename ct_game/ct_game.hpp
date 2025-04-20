#pragma once

#include <array>

constexpr int kBoardSize = 3;
constexpr int kFlatBoardSize = kBoardSize * kBoardSize;
constexpr int kDirecions[4] = {+1, -1, +3, -3};

constexpr bool IsValid(const std::array<int, kFlatBoardSize> &board) {
  bool seen[kFlatBoardSize] = {};
  for (int x : board) {
    if (x < 0 || x >= kFlatBoardSize) {
      return false;
    }
    if (seen[x]) {
      return false;
    }
    seen[x] = true;
  }
  return true;
}

constexpr bool IsSolvable(const std::array<int, kFlatBoardSize> &board) {
  int inversions_count = 0;
  for (int i = 0; i < kFlatBoardSize; ++i) {
    if (board[i] == 0) {
      continue;
    }
    for (int j = i + 1; j < kFlatBoardSize; ++j) {
      if (board[j] != 0 && board[i] > board[j]) {
        ++inversions_count;
      }
    }
  }
  return (inversions_count % 2) == 0;
}

constexpr int Absolute(int x) { return x < 0 ? -x : x; }

constexpr int
CalculateHeuristics(const std::array<int, kFlatBoardSize> &board) {
  int dist = 0;
  for (int i = 0; i < kFlatBoardSize; ++i) {
    int value = board[i];
    if (value == 0) {
      continue;
    }
    int target_x = (value - 1) / kBoardSize;
    int target_y = (value - 1) % kBoardSize;
    int current_x = i / kBoardSize;
    int current_y = i % kBoardSize;
    dist += Absolute(current_x - target_x) + Absolute(current_y - target_y);
  }
  return dist;
}

constexpr int MoveZero(int zero_positon, int direction) {
  int new_zero_position = zero_positon + kDirecions[direction];
  if (direction < 2) {
    if (new_zero_position < 0 || new_zero_position >= kFlatBoardSize) {
      return -1;
    }
    if ((zero_positon / kBoardSize) != (new_zero_position / kBoardSize)) {
      return -1;
    }
  } else {
    if (new_zero_position < 0 || new_zero_position >= kFlatBoardSize) {
      return -1;
    }
  }
  return new_zero_position;
}

template <int MaxDepth> struct Solver {
  const std::array<int, kFlatBoardSize> &board;
  int init_zero_position;

  constexpr Solver(const std::array<int, kFlatBoardSize> &board,
                   int zero_position)
      : board(board), init_zero_position(zero_position) {}

  constexpr bool DFS(std::array<int, kFlatBoardSize> &current_state,
                     int zero_positon, int g, int bound,
                     int last_direction) const {
    int h = CalculateHeuristics(current_state);
    if (g + h > bound) {
      return false;
    }

    if (h == 0) {
      return true;
    }

    for (int direction = 0; direction < 4; ++direction) {
      if ((direction ^ 1) == last_direction) {
        continue;
      }
      int new_zero_position = MoveZero(zero_positon, direction);
      if (new_zero_position < 0) {
        continue;
      }

      int tmp = current_state[new_zero_position];
      current_state[new_zero_position] = 0;
      current_state[zero_positon] = tmp;

      if (DFS(current_state, new_zero_position, g + 1, bound, direction)) {
        return true;
      }

      current_state[zero_positon] = 0;
      current_state[new_zero_position] = tmp;
    }
    return false;
  }

  constexpr int Solve() const {
    std::array<int, kFlatBoardSize> current_state = board;
    int bound = CalculateHeuristics(current_state);
    if (bound == 0) {
      return 0;
    }
    for (int depth = bound; depth <= MaxDepth; ++depth) {
      if (DFS(current_state, init_zero_position, 0, depth, -1)) {
        return depth;
      }
    }
    return -1;
  }
};

constexpr int SolutionLength(const std::array<int, kFlatBoardSize> &board) {
  if (!IsValid(board)) {
    return -2;
  }
  if (!IsSolvable(board)) {
    return -1;
  }

  int zero_position = 0;
  for (int i = 0; i < kFlatBoardSize; ++i) {
    if (board[i] == 0) {
      zero_position = i;
      break;
    }
  }

  constexpr int kMaxDepth = 31;
  return Solver<kMaxDepth>(board, zero_position).Solve();
}
