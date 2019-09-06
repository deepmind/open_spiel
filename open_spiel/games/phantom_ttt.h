// Copyright 2019 DeepMind Technologies Ltd. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef THIRD_PARTY_OPEN_SPIEL_GAMES_PHANTOM_TTT_H_
#define THIRD_PARTY_OPEN_SPIEL_GAMES_PHANTOM_TTT_H_

#include <array>
#include <map>
#include <string>
#include <vector>

#include "./tic_tac_toe.h"
#include "open_spiel/spiel.h"

// Phantom Tic-Tac-Toe is a phantom version of the classic game of Tic-Tac-Toe
// (Noughts and Crosses). For some perfect information game X", the game
// "phantom X" is a version of the game X where the players do not observe the
// other players' pieces. Only a referee knows the full state of the board.
// So, on a player's turn, a chosen moves may fail because it is illegal given
// the true state of the board; in this case, a player can continue to try moves
// until one succeeds.
//
// Common phantom games include Kriegspiel (Phantom chess), e.g. see
// https://en.wikipedia.org/wiki/Kriegspiel_(chess), and Phantom Go.
// See also http://mlanctot.info/files/papers/PhD_Thesis_MarcLanctot.pdf, Ch 3.
//
// Parameters:
///    "obstype", string, "reveal-nothing" (default) or "reveal-numturns"

namespace open_spiel {
namespace phantom_ttt {

constexpr const char* kDefaultObsType = "reveal-nothing";

// Longest sequence is 17 moves, e.g. 0011223344556677889
constexpr int kLongestSequence = 2 * tic_tac_toe::kNumCells - 1;
constexpr int kBitsPerAction = 10;  // Reserve 9 as "I don't know."

enum class ObservationType {
  kRevealNothing,
  kRevealNumTurns,
};

// State of an in-play game.
class PhantomTTTState : public State {
 public:
  PhantomTTTState(int num_distinct_actions, ObservationType obs_type);

  // Forward to underlying game state
  int CurrentPlayer() const override { return state_.CurrentPlayer(); }
  std::string ActionToString(int player, Action action_id) const {
    return state_.ActionToString(player, action_id);
  }
  std::string ToString() const override { return state_.ToString(); }
  bool IsTerminal() const override { return state_.IsTerminal(); }
  std::vector<double> Returns() const override { return state_.Returns(); }
  std::string Observation(int player) const override {
    return state_.Observation(player);
  }
  void ObservationAsNormalizedVector(
      int player, std::vector<double>* values) const override {
    state_.ObservationAsNormalizedVector(player, values);
  }

  // These are implemented for phantom games
  std::string InformationState(int player) const override;
  void InformationStateAsNormalizedVector(
      int player, std::vector<double>* values) const override;
  std::unique_ptr<State> Clone() const override;
  void UndoAction(int player, Action move) override;
  std::vector<Action> LegalActions() const override;

 protected:
  void DoApplyAction(Action move) override;

 private:
  std::string ViewToString(int player) const;
  std::string ActionSequenceToString(int player) const;

  tic_tac_toe::TicTacToeState state_;
  ObservationType obs_type_;
  // TODO(author2): Use the base class history_ instead.
  std::vector<std::pair<int, Action>> action_sequence_;
  std::array<tic_tac_toe::CellState, tic_tac_toe::kNumCells> x_view_;
  std::array<tic_tac_toe::CellState, tic_tac_toe::kNumCells> o_view_;
};

// Game object.
class PhantomTTTGame : public Game {
 public:
  explicit PhantomTTTGame(const GameParameters& params);
  std::unique_ptr<State> NewInitialState() const override {
    return std::unique_ptr<State>(
        new PhantomTTTState(NumDistinctActions(), obs_type_));
  }
  int NumDistinctActions() const override { return game_.NumDistinctActions(); }
  int NumPlayers() const override { return game_.NumPlayers(); }
  double MinUtility() const override { return game_.MinUtility(); }
  double UtilitySum() const override { return game_.UtilitySum(); }
  double MaxUtility() const override { return game_.MaxUtility(); }
  std::vector<int> ObservationNormalizedVectorShape() const {
    return game_.ObservationNormalizedVectorShape();
  }

  std::unique_ptr<Game> Clone() const override {
    return std::unique_ptr<Game>(new PhantomTTTGame(*this));
  }
  // This will depend on the obstype parameter.
  std::vector<int> InformationStateNormalizedVectorShape() const override;
  int MaxGameLength() const { return kLongestSequence; }

 private:
  tic_tac_toe::TicTacToeGame game_;
  ObservationType obs_type_;
};

inline std::ostream& operator<<(std::ostream& stream,
                                const ObservationType& obs_type) {
  switch (obs_type) {
    case ObservationType::kRevealNothing:
      return stream << "Reveal Nothing";
    case ObservationType::kRevealNumTurns:
      return stream << "Reveal Num Turns";
    default:
      SpielFatalError("Unknown observation type");
  }
}

}  // namespace phantom_ttt
}  // namespace open_spiel

#endif  // THIRD_PARTY_OPEN_SPIEL_GAMES_PHANTOM_TTT_H_
