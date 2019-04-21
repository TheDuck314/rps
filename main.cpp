#include "networking/BotError.h"
#include "networking/Connection.h"

#include <array>
#include <cstdio>
#include <sstream>
#include <memory>
#include <vector>

struct GameResult {
    std::array<std::vector<std::string>, 2> moves;
    std::array<int, 2> points{0, 0};
    int winner = -1;
    std::string message;

    std::string to_json() {
        std::ostringstream ss;
        ss << "{\n";
        // moves of each player, as an array of two arrays
        ss << "  moves: [\n";
        for (int i = 0; i < 2; ++i) {
            ss << "    [";
            for (const std::string &m : moves[i]) {
                ss << "\"" << m << "\", ";
            }
            ss << "],\n";
        }
        ss << "  ],\n";
        // points as an array of two ints
        ss << "  points: [" << points[0] << ", " << points[1] << "],\n";
        ss << "  winner: " << winner << ",\n";
        ss << "  message: \"" << message << "\",\n";
        ss << "}\n";
        return ss.str();
    }
};

class Game {
  public:
    GameResult result;

    Game(const std::string &command1, const std::string &command2, std::chrono::milliseconds timeout)
        : timeout(timeout) {
        player_conns = { 
            std::make_unique<Connection>(command1),
            std::make_unique<Connection>(command2)
        };
    }

    bool turn() {
        std::array<std::string, 2> moves;
        for (int p = 0; p < 2; ++p) {
            bool valid = get_move(p, moves[p]);
            if (!valid) {
                result.winner = 1 - p;
                result.message = "Some sort of error happened.";
                return false;
            }
        }

        // both moves were valid
        for (int p = 0; p < 2; ++p) {
            result.moves[p].push_back(moves[p]);
        }

        if (moves[0] == moves[1]) {
            // nobody gets any points
        } else if ((moves[0] == "R" && moves[1] == "S") ||
            (moves[0] == "S" && moves[1] == "P") ||
            (moves[0] == "P" && moves[1] == "R")) {
            ++result.points[0];
        } else {
            ++result.points[1];
        }
        
        return true;
    }

    void finish() {
        if (result.winner == -1) {
            if (result.points[0] >= result.points[1]) {
                result.winner = 0;
            } else {
                result.winner = 1;
            }
        }
    }

  private:
    bool validate_move(const std::string &move) {
        return (move == "R") || (move == "P") || (move == "S");
    }

    bool get_move(int player, std::string &move) {
        try {
            move = player_conns[player]->get_line(timeout);
        } catch (BotError &e) {
            printf("Error getting move for player %d: %s\n", player, e.what());
            return false;
        }

        if (!validate_move(move)) {
            printf("Invalid move by player %d: '%s'\n", player, move.c_str());
            return false;
        }

        return true;
    }

    std::array<std::unique_ptr<Connection>, 2> player_conns;
    std::chrono::milliseconds timeout;
};


int main(int argc, char **argv) {
    if (argc != 3) {
        printf("usage: rps 'player 1 command' 'player 2 command'\n");
        return 1;
    }

    const std::chrono::milliseconds timeout(1000);
    Game game(argv[1], argv[2], timeout);

    for (int i = 0; i < 10; ++i) {
        if (!game.turn()) break;
    }

    game.finish();

    printf("%s\n", game.result.to_json().c_str());

    return 0;
}