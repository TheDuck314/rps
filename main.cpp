#include "networking/BotError.h"
#include "networking/Connection.h"

#include <array>
#include <cstdarg>
#include <cstdio>
#include <sstream>
#include <memory>
#include <vector>

#include <tclap/CmdLine.h>


std::string stringf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char *buf;
    vasprintf(&buf, format, args);
    std::string ret(buf);
    free(buf);
    return ret;
}

struct GameResult {
    std::array<std::vector<std::string>, 2> moves;
    std::array<int, 2> points{0, 0};
    int winner = -1;
    std::string message;

    std::string to_json() {
        std::ostringstream ss;
        ss << "{\n";
        // moves of each player, as an array of two arrays
        ss << "  \"moves\": [\n";
        for (int i = 0; i < 2; ++i) {
            ss << "    [";
            for (size_t m = 0; m < moves[i].size(); ++m) {
                ss << "\"" << moves[i][m] << "\"";
                if (m < moves[i].size() - 1) {
                    ss << ", ";
                }
            }
            ss << "]";
            if (i == 0) {
                ss << ",";
            }
            ss << "\n";
        }
        ss << "  ],\n";
        // points as an array of two ints
        ss << "  \"points\": [" << points[0] << ", " << points[1] << "],\n";
        ss << "  \"winner\": " << winner << ",\n";
        ss << "  \"message\": \"" << message << "\"\n";
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
            if (!get_move(p, moves[p])) {
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

        // tell each player the other player's move
        for (int p = 0; p < 2; ++p) {
            if (!send_move(p, moves[1-p])) {
                return false;
            }
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
            fprintf(stderr, "Error getting move for player %d: %s\n", player, e.what());
            fprintf(stderr, "bot stderr: %s\n", player_conns[player]->get_errors().c_str());
            result.winner = 1 - player;
            result.message = stringf("error getting move for player %d", player);
            return false;
        }

        if (!validate_move(move)) {
            fprintf(stderr, "Invalid move by player %d: '%s'\n", player, move.c_str());
            result.winner = 1 - player;
            result.message = stringf("invalid move by player player %d", player);
            return false;
        }

        return true;
    }

    bool send_move(int player, const std::string &move) {
        try {
            player_conns[player]->send_string(move + "\n", timeout);
        } catch (BotError &e) {
            fprintf(stderr, "Error sending move to player %d: %s\n", player, e.what());
            fprintf(stderr, "bot stderr: %s\n", player_conns[player]->get_errors().c_str());
            result.winner = 1 - player;
            result.message = stringf("error sending move to player %d", player);
            return false;
        }
        return true;
    }

    std::array<std::unique_ptr<Connection>, 2> player_conns;
    std::chrono::milliseconds timeout;
};


int main(int argc, char **argv) {
    std::string cmd1;
    std::string cmd2;
    long max_turns;

    try {
        TCLAP::CmdLine cmd_line("Command description message", ' ', "0.9");
        TCLAP::UnlabeledValueArg<std::string> cmd1_arg("cmd1", "executable command", true, "", "player 1 command", cmd_line);
        TCLAP::UnlabeledValueArg<std::string> cmd2_arg("cmd2", "executable command", true, "", "player 1 command", cmd_line);
        TCLAP::SwitchArg csv_arg("c", "csv", "csv output instead of json", cmd_line, false);
        TCLAP::ValueArg<long> turns_arg("t", "turns", "how many turns to play", false, 100, "integer", cmd_line);
        
        cmd_line.parse(argc, argv);
        
        cmd1       = cmd1_arg.getValue();
        cmd2       = cmd2_arg.getValue();
        max_turns  = turns_arg.getValue();
    } catch (TCLAP::ArgException &e)  {
        fprintf(stderr, "error: %s for arg %s\n", e.error().c_str(), e.argId().c_str());
        return 1;
    }

    const std::chrono::milliseconds timeout(1000);
    Game game(cmd1, cmd2, timeout);

    for (long i = 0; i < max_turns; ++i) {
        fprintf(stderr, "turn %ld\n", i);
        if (!game.turn()) break;
    }

    game.finish();

    puts(game.result.to_json().c_str());

    return 0;
}
