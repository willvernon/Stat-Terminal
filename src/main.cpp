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
    std::string name;
    std::string team;
    std::string position;
    int age;
    std::string height;
    int weight;
    int passing_yards;
    int touchdowns;
    int interceptions;
    float completion_pct;
    int rushing_yards;
    int games_played;
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
    error_msg.clear();
    current_stats = PlayerStats{};

    sqlite3 *db;
    int rc = sqlite3_open("../db/stat-term.db", &db);
    if (rc != SQLITE_OK) {
        error_msg = "Can't open database: " + std::string(sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    sqlite3_stmt *stmt;
    // const char *query = "SELECT player_name, age, team, pos, g, gs, mp, fg, fga, fg_pct, three_p, three_pa, three_p_pct, two_p, two_pa, two_p_pct, efg_pct, ft, fta, ft_pct, orb, drb, trb, ast, stl, blk, tov, pf, pts, awards;"
    //                     "FROM nba_season_player_stats WHERE player_name = ?";
	const char *query = "SELECT *"
	"FROM nba_season_player_stats WHERE player_name = ?";
    rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        error_msg = "SQL error: " + std::string(sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_text(stmt, 1, player_name_input.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        current_stats.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        current_stats.team = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        current_stats.position = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        current_stats.age = sqlite3_column_int(stmt, 3);
        current_stats.height = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
        current_stats.weight = sqlite3_column_int(stmt, 5);
        current_stats.passing_yards = sqlite3_column_int(stmt, 6); // Using pts as a placeholder
        current_stats.games_played = sqlite3_column_int(stmt, 11);
    } else {
        error_msg = "No player found: " + player_name_input;
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
            text("Player: " + current_stats.name) | bold,
            text("Team: " + current_stats.team),
            text("Position: " + current_stats.position),
            text("Age: " + std::to_string(current_stats.age)),
            text("Height: " + current_stats.height),
            text("Weight: " + std::to_string(current_stats.weight) + " lbs"),
        }) | border;
    };

    auto stats_display = [&]() {
        if (current_stats.name.empty()) {
            if (!error_msg.empty()) return text(error_msg) | color(Color::Red) | center;
            return text("No player selected") | center;
        }
        return vbox({
            text("Player Stats") | bold,
            separator(),
            hbox({text("Points: " + std::to_string(current_stats.passing_yards))}),
            hbox({text("Games Played: " + std::to_string(current_stats.games_played))}),
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