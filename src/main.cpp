#include "ftxui/component/captured_mouse.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/util/ref.hpp"
#include "sqlite3.h"
#include <iostream>
#include <string>
#include <vector>

using namespace ftxui;

struct PlayerStats {
    std::string season;        // TEXT: Season (e.g., "2023")
    int rk;                    // INTEGER: Rank
    std::string player_name;   // TEXT: Player name
    int age;                   // INTEGER: Age
    std::string team;          // TEXT: Team abbreviation
    std::string pos;           // TEXT: Position
    int g;                     // INTEGER: Games played
    int gs;                    // INTEGER: Games started
    float mp;                  // REAL: Minutes played
    int fg;                    // INTEGER: Field goals made
    int fga;                   // INTEGER: Field goals attempted
    float fg_pct;              // REAL: Field goal percentage
    int three_p;               // INTEGER: 3-point field goals made
    int three_pa;              // INTEGER: 3-point field goals attempted
    float three_p_pct;         // REAL: 3-point field goal percentage
    int two_p;                 // INTEGER: 2-point field goals made
    int two_pa;                // INTEGER: 2-point field goals attempted
    float two_p_pct;           // REAL: 2-point field goal percentage
    float efg_pct;             // REAL: Effective field goal percentage
    float ft;                  // REAL: Free throws made (stored as float per schema)
    int fta;                   // INTEGER: Free throws attempted
    float ft_pct;              // REAL: Free throw percentage
    int orb;                   // INTEGER: Offensive rebounds
    int drb;                   // INTEGER: Defensive rebounds
    int trb;                   // INTEGER: Total rebounds
    int ast;                   // INTEGER: Assists
    int stl;                   // INTEGER: Steals
    int blk;                   // INTEGER: Blocks
    int tov;                   // INTEGER: Turnovers
    int pf;                    // INTEGER: Personal fouls
    int pts;                   // INTEGER: Points
    std::string awards;        // TEXT: Awards (e.g., "All-NBA")
};

ButtonOption Style() {
    auto option = ButtonOption::Animated();
    option.transform = [](const EntryState &s) {
        auto element = text(s.label);
        if (s.focused) element |= bold;
        return element | center | size(WIDTH, EQUAL, 15) | size(HEIGHT, EQUAL, 1);
    };
    return option;
}

void performSearch(const std::string &player_name_input, PlayerStats &current_stats, std::string &error_msg) {
    std::cerr << "Searching for: " << player_name_input << std::endl;
    error_msg.clear();
    current_stats = PlayerStats{}; // Reset stats

    sqlite3 *db;
    int rc = sqlite3_open("../db/stat-term.db", &db);
    if (rc != SQLITE_OK) {
        error_msg = "Can't open database: " + std::string(sqlite3_errmsg(db));
        std::cerr << error_msg << std::endl;
        sqlite3_close(db);
        return;
    }

    sqlite3_stmt *stmt;
    const char *query = 
        "SELECT season, rk, player_name, age, team, pos, g, gs, mp, "
        "fg, fga, fg_pct, three_p, three_pa, three_p_pct, two_p, two_pa, two_p_pct, "
        "efg_pct, ft, fta, ft_pct, orb, drb, trb, ast, stl, blk, tov, pf, pts, awards "
        "FROM nba_season_player_stats WHERE player_name = ?";
    rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        error_msg = "SQL error: " + std::string(sqlite3_errmsg(db));
        std::cerr << error_msg << std::endl;
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_text(stmt, 1, player_name_input.c_str(), -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        current_stats.season = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        current_stats.rk = sqlite3_column_int(stmt, 1);
        current_stats.player_name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        current_stats.age = sqlite3_column_int(stmt, 3);
        current_stats.team = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
        current_stats.pos = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
        current_stats.g = sqlite3_column_int(stmt, 6);
        current_stats.gs = sqlite3_column_int(stmt, 7);
        current_stats.mp = static_cast<float>(sqlite3_column_double(stmt, 8));
        current_stats.fg = sqlite3_column_int(stmt, 9);
        current_stats.fga = sqlite3_column_int(stmt, 10);
        current_stats.fg_pct = static_cast<float>(sqlite3_column_double(stmt, 11));
        current_stats.three_p = sqlite3_column_int(stmt, 12);
        current_stats.three_pa = sqlite3_column_int(stmt, 13);
        current_stats.three_p_pct = static_cast<float>(sqlite3_column_double(stmt, 14));
        current_stats.two_p = sqlite3_column_int(stmt, 15);
        current_stats.two_pa = sqlite3_column_int(stmt, 16);
        current_stats.two_p_pct = static_cast<float>(sqlite3_column_double(stmt, 17));
        current_stats.efg_pct = static_cast<float>(sqlite3_column_double(stmt, 18));
        current_stats.ft = static_cast<float>(sqlite3_column_double(stmt, 19));
        current_stats.fta = sqlite3_column_int(stmt, 20);
        current_stats.ft_pct = static_cast<float>(sqlite3_column_double(stmt, 21));
        current_stats.orb = sqlite3_column_int(stmt, 22);
        current_stats.drb = sqlite3_column_int(stmt, 23);
        current_stats.trb = sqlite3_column_int(stmt, 24);
        current_stats.ast = sqlite3_column_int(stmt, 25);
        current_stats.stl = sqlite3_column_int(stmt, 26);
        current_stats.blk = sqlite3_column_int(stmt, 27);
        current_stats.tov = sqlite3_column_int(stmt, 28);
        current_stats.pf = sqlite3_column_int(stmt, 29);
        current_stats.pts = sqlite3_column_int(stmt, 30);
        current_stats.awards = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 31));
        std::cerr << "Found: " << current_stats.player_name << ", " << current_stats.team << std::endl;
    } else {
        error_msg = "No player found: " + player_name_input;
        std::cerr << "Step returned: " << rc << " (100=ROW, 101=DONE)" << std::endl;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

int main() {
    std::string player_name_input;
    PlayerStats current_stats;
    std::string error_msg;
    int current_screen = 0;

    InputOption input_option;
    input_option.on_enter = [&] {
        performSearch(player_name_input, current_stats, error_msg);
        if (!error_msg.empty()) current_screen = 0;
        else current_screen = 1;
    };
    auto input_player_name = Input(&player_name_input, "Enter player name", input_option);

    int toggle_selected = 0;
    std::vector<std::string> toggle_entries = {"Any Sport", "NFL", "NBA", "NHL"};
    auto sport_toggle = Toggle(&toggle_entries, &toggle_selected);

    auto submit_button = Button("Search", [&] {
        if (!player_name_input.empty()) {
            performSearch(player_name_input, current_stats, error_msg);
            if (!error_msg.empty()) current_screen = 0;
            else current_screen = 1;
        }
    }, Style());

    auto back_button = Button("<-", [&] { current_screen = 0; }, Style());

    auto component = Container::Horizontal({
        sport_toggle,
        input_player_name,
        submit_button,
        back_button,
    });

    auto event_handler = CatchEvent(component, [&](Event event) {
        if (event == Event::Return && input_player_name->Focused()) {
            performSearch(player_name_input, current_stats, error_msg);
            if (!error_msg.empty()) current_screen = 0;
            else current_screen = 1;
            player_name_input.clear();
            return true;
        }
        return false;
    });

    auto welcome = []() {
        return vbox({
            text(L"   ______       __    ______              _          __"),
            text(L"  / __/ /____ _/ /_  /_  __/__ ______ _  (_)__  ___ _/ /"),
            text(L" _\\ \\/ __/ _ `/ __/   / / / -_) __/  ' \\/ / _ \\/ _ `/ /"),
            text(L"/___/\\__/\\_,_/\\__/   /_/  \\__/_/ /_/_/_/_/_//_/\\_,_/_/"),
        }) | border;
    };

    auto player_info = [&]() {
        return vbox({
            text("Player: " + current_stats.player_name) | bold,
            text("Team: " + current_stats.team),
            text("Position: " + current_stats.pos),
            text("Age: " + std::to_string(current_stats.age)),
        }) | border;
    };

    auto stats_display = [&]() {
        if (current_stats.player_name.empty()) {
            if (!error_msg.empty()) {
                return text(error_msg) | color(Color::Red) | center;
            }
            return text("No player selected") | center;
        }
        return vbox({
            text("Player Stats - " + current_stats.season) | bold,
            separator(),
            hbox({text("Games: " + std::to_string(current_stats.g))}),
            hbox({text("Minutes: " + std::to_string(current_stats.mp))}),
            hbox({text("Points: " + std::to_string(current_stats.pts))}),
            hbox({text("FG: " + std::to_string(current_stats.fg) + "/" + std::to_string(current_stats.fga) + " (" + std::to_string(current_stats.fg_pct) + "%)")}),
            hbox({text("3P: " + std::to_string(current_stats.three_p) + "/" + std::to_string(current_stats.three_pa) + " (" + std::to_string(current_stats.three_p_pct) + "%)")}),
            hbox({text("FT: " + std::to_string(current_stats.ft) + "/" + std::to_string(current_stats.fta) + " (" + std::to_string(current_stats.ft_pct) + "%)")}),
            hbox({text("Rebounds: " + std::to_string(current_stats.trb) + " (O: " + std::to_string(current_stats.orb) + ", D: " + std::to_string(current_stats.drb) + ")")}),
            hbox({text("Assists: " + std::to_string(current_stats.ast))}),
            hbox({text("Steals: " + std::to_string(current_stats.stl))}),
            hbox({text("Blocks: " + std::to_string(current_stats.blk))}),
            hbox({text("Turnovers: " + std::to_string(current_stats.tov))}),
            hbox({text("Fouls: " + std::to_string(current_stats.pf))}),
            hbox({text("Awards: " + current_stats.awards)}),
        }) | border | flex;
    };

    auto renderer = Renderer(event_handler, [&] {
        if (current_screen == 0) {
            return vbox({
                welcome(),
                hbox({text("Select a league: "), sport_toggle->Render()}) | hcenter | vcenter | border | xflex,
                hbox({text("Player: ") | vcenter, input_player_name->Render() | vcenter | size(WIDTH, GREATER_THAN, 20), submit_button->Render()}) | hcenter | border,
                error_msg.empty() ? text("") : text(error_msg) | color(Color::Red) | center,
            }) | vcenter | hcenter | border;
        } else {
            return gridbox({
                {
                    vbox({player_info() | size(HEIGHT, GREATER_THAN, 10)}) | size(WIDTH, EQUAL, 22),
                    vbox({
                        hbox({back_button->Render(), text("Search New Player: "), input_player_name->Render() | vcenter | flex, submit_button->Render()}) | border,
                        stats_display() | flex_grow,
                    }) | flex_grow,
                },
            }) | size(HEIGHT, GREATER_THAN, 40) | border;
        }
    });

    auto screen = ScreenInteractive::Fullscreen();
    screen.Loop(renderer);
    return 0;
}