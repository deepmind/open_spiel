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


#include <cstdlib>
#include <string>
#include <memory>


#include "open_spiel/game_transforms/efg_writer.h"
#include "open_spiel/utils/file.h"
#include "open_spiel/spiel_utils.h"
#include "open_spiel/game_transforms/turn_based_simultaneous_game.h"
#include "open_spiel/tests/basic_tests.h"

namespace open_spiel {

void WriteAndLoadGame(std::string game_name) {
  std::string tmp_dir = file::GetTmpDir();
  std::string dir = tmp_dir + "open_spiel-test";
  std::string filename = dir + "/" + game_name + ".efg";

  SPIEL_CHECK_TRUE(file::Exists(tmp_dir));
  SPIEL_CHECK_TRUE(file::IsDirectory(tmp_dir));

  std::shared_ptr<const Game> base_game = LoadGame(game_name);

  EFGWriter writer = EFGWriter(*base_game, filename);
  writer.Write();

  std::shared_ptr<const Game> efg_game =
      LoadGame("efg_game", {{"filename", GameParameter(filename)}});
  SPIEL_CHECK_TRUE(efg_game != nullptr);
  GameType base_game_type = base_game->GetType();
  GameType efg_game_type = efg_game->GetType();
  SPIEL_CHECK_EQ(efg_game_type.dynamics, GameType::Dynamics::kSequential);
  SPIEL_CHECK_EQ(efg_game_type.information, base_game_type.information);
  SPIEL_CHECK_EQ(efg_game_type.utility, base_game_type.utility);
  SPIEL_CHECK_EQ(efg_game_type.chance_mode, base_game_type.chance_mode);
  SPIEL_CHECK_EQ(efg_game->NumDistinctActions(),
                 base_game->NumDistinctActions());
  testing::RandomSimTestNoSerialize(*efg_game, 100);

  SPIEL_CHECK_TRUE(file::Exists(filename));
  SPIEL_CHECK_TRUE(file::Remove(filename));
  SPIEL_CHECK_FALSE(file::Remove(filename));  // Already gone
  SPIEL_CHECK_FALSE(file::Exists(filename));
}

}  // namespace open_spiel

int main(int argc, char **argv) {
  open_spiel::WriteAndLoadGame("kuhn_poker");
  open_spiel::WriteAndLoadGame("leduc_poker");
  open_spiel::WriteAndLoadGame("liars_dice");
}
