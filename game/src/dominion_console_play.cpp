#include "open_spiel/spiel.h"
#include "open_spiel/spiel_bots.h"
#include "open_spiel/tests/console_play_test.h"
#include <unordered_map>

int main() {
  auto game = open_spiel::LoadGame("dominion");
  std::unordered_map<open_spiel::Player, std::unique_ptr<open_spiel::Bot>> bots;
  bots[1] = open_spiel::MakeUniformRandomBot(1, 12345);
  open_spiel::testing::ConsolePlayTest(*game, nullptr, nullptr, &bots);
  return 0;
}
