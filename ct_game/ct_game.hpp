#include <algorithm>
#include <cmath>
#include <iostream>
#include <queue>
#include <unordered_set>
#include <vector>

template <typename T>
using Matrix = std::vector<std::vector<T>>;

template <typename T>
using PriorityQueue = std::priority_queue<T, std::vector<T>, std::greater<T>>;

using StrUnorderedSet = std::unordered_set<std::string>;

using PairVector = std::vector<std::pair<int, int>>;

struct Puzzle {
  Matrix<int> board;
  int x;     // x coordinate of empty cell
  int y;     // y coordinate of empty cell
  int dist;  // sum of Manhattan distances
  std::string path;

  Puzzle(Matrix<int> b, int xx, int yy, int d, std::string p)
      : board(b), x(xx), y(yy), dist(d), path(p) {}


  bool operator>(const Puzzle& other) const {
    return (path.size() + dist) > (other.path.size() + other.dist);
  }
};

const int kBoardSize = 3;
const int kDirectionsNumber = 4;
const Matrix<int> kGoal = {{1, 2, 3}, {4, 5, 6}, {7, 8, 0}};
const PairVector kDirections = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
const std::string kDirectionsLetters = "RLDU";
const std::string kEmptyString = "";

Matrix<int> InputBoard() {
  Matrix<int> board(kBoardSize, std::vector<int>(kBoardSize, 0));
  for (int i = 0; i < kBoardSize; ++i) {
    for (int j = 0; j < kBoardSize; ++j) {
      std::cin >> board[i][j];
    }
  }
  return board;
}

int CalculateDist(const Matrix<int>& board) {
  int dist = 0;
  for (int i = 0; i < kBoardSize; ++i) {
    for (int j = 0; j < kBoardSize; ++j) {
      if (board[i][j] == 0) {
        continue;
      }
      int target_x = (board[i][j] - 1) / kBoardSize;
      int target_y = (board[i][j] - 1) % kBoardSize;
      dist += std::abs(i - target_x) + std::abs(j - target_y);
    }
  }
  return dist;
}

bool IsSolvable(const Matrix<int>& board) {
  std::vector<int> board_flat;
  for (const auto& row : board) {
    for (const auto& cell : row) {
      if (cell != 0) {
        board_flat.push_back(cell);
      }
    }
  }
  int sum_of_bigger_before = 0;
  for (size_t i = 0; i < board_flat.size(); ++i) {
    for (size_t j = i + 1; j < board_flat.size(); ++j) {
      if (board_flat[i] > board_flat[j]) {
        ++sum_of_bigger_before;
      }
    }
  }
  return sum_of_bigger_before % 2 == 0;
}

void SolvePuzzle(const Matrix<int>& start) {
  PriorityQueue<Puzzle> pq;
  StrUnorderedSet visited;

  int start_x = 0;
  int start_y = 0;
  for (int i = 0; i < kBoardSize; ++i) {
    for (int j = 0; j < kBoardSize; ++j) {
      if (start[i][j] == 0) {
        start_x = i;
        start_y = j;
      }
    }
  }

  pq.emplace(start, start_x, start_y, CalculateDist(start), kEmptyString);

  while (!pq.empty()) {
    Puzzle current = pq.top();
    pq.pop();

    if (current.board == kGoal) {
      std::cout << current.path.size() << '\n';
      std::cout << current.path << '\n';
      return;
    }

    std::string current_state;
    for (const auto& row : current.board) {
      for (const auto& cell : row) {
        current_state += std::to_string(cell);
      }
    }

    if (visited.find(current_state) != visited.end()) {
      continue;
    }
    visited.insert(current_state);

    for (int i = 0; i < kDirectionsNumber; ++i) {
      int new_x = current.x + kDirections[i].first;
      int new_y = current.y + kDirections[i].second;

      if (new_x >= 0 && new_x < kBoardSize && new_y >= 0 &&
          new_y < kBoardSize) {
        Matrix<int> new_board = current.board;
        std::swap(new_board[current.x][current.y], new_board[new_x][new_y]);
        pq.emplace(new_board, new_x, new_y, CalculateDist(new_board),
                   current.path + kDirectionsLetters[i]);
      }
    }
  }
}

int main() {
  Matrix<int> start = InputBoard();

  if (IsSolvable(start)) {
    SolvePuzzle(start);
  } else {
    std::cout << -1 << '\n';
  }
}
