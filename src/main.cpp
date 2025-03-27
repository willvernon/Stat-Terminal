#include "ftxui/component/captured_mouse.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/util/ref.hpp"
#include "sqlite3.h"
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace ftxui;

struct PlayerStats {
	std::string season;
	int rk;
	std::string player_name;
	int age;
	std::string team;
	std::string pos;
	int g;
	int gs;
	float mp;
	int fg;
	int fga;
	float fg_pct;
	int three_p;
	int three_pa;
	float three_p_pct;
	int two_p;
	int two_pa;
	float two_p_pct;
	float efg_pct;
	float ft;
	int fta;
	float ft_pct;
	int orb;
	int drb;
	int trb;
	int ast;
	int stl;
	int blk;
	int tov;
	int pf;
	int pts;
	std::string awards;
};

std::vector<PlayerStats>
	player_seasons; // Global vector instead of single PlayerStats

ButtonOption Style() {
	// Your existing Style function remains unchanged
	auto option = ButtonOption::Animated();
	option.transform = [](const EntryState &s) {
		auto element = text(s.label);
		if (s.focused)
			element |= bold;
		return element | center | size(WIDTH, EQUAL, 13) |
			   size(HEIGHT, EQUAL, 1);
	};
	return option;
}

void performSearch(const std::string &player_name_input,
				   std::string &error_msg) {
	std::cerr << "Searching for: " << player_name_input << std::endl;
	error_msg.clear();
	player_seasons.clear(); // Clear previous results

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
		"fg, fga, fg_pct, three_p, three_pa, three_p_pct, two_p, two_pa, "
		"two_p_pct, efg_pct, ft, fta, ft_pct, orb, drb, trb, ast, stl, blk, "
		"tov, pf, pts, awards "
		"FROM nba_season_player_stats WHERE player_name = ? "
		"ORDER BY season DESC";

	rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		error_msg = "SQL error: " + std::string(sqlite3_errmsg(db));
		std::cerr << error_msg << std::endl;
		sqlite3_close(db);
		return;
	}

	sqlite3_bind_text(stmt, 1, player_name_input.c_str(), -1, SQLITE_STATIC);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		PlayerStats stats;
		stats.season =
			sqlite3_column_text(stmt, 0)
				? reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0))
				: "";
		stats.rk = sqlite3_column_int(stmt, 1);
		stats.player_name =
			sqlite3_column_text(stmt, 2)
				? reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2))
				: "";
		stats.age = sqlite3_column_int(stmt, 3);
		stats.team =
			sqlite3_column_text(stmt, 4)
				? reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4))
				: "";
		stats.pos =
			sqlite3_column_text(stmt, 5)
				? reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5))
				: "";
		stats.g = sqlite3_column_int(stmt, 6);
		stats.gs = sqlite3_column_int(stmt, 7);
		stats.mp = static_cast<float>(sqlite3_column_double(stmt, 8));
		stats.fg = sqlite3_column_int(stmt, 9);
		stats.fga = sqlite3_column_int(stmt, 10);
		stats.fg_pct = static_cast<float>(sqlite3_column_double(stmt, 11));
		stats.three_p = sqlite3_column_int(stmt, 12);
		stats.three_pa = sqlite3_column_int(stmt, 13);
		stats.three_p_pct = static_cast<float>(sqlite3_column_double(stmt, 14));
		stats.two_p = sqlite3_column_int(stmt, 15);
		stats.two_pa = sqlite3_column_int(stmt, 16);
		stats.two_p_pct = static_cast<float>(sqlite3_column_double(stmt, 17));
		stats.efg_pct = static_cast<float>(sqlite3_column_double(stmt, 18));
		stats.ft = static_cast<float>(sqlite3_column_double(stmt, 19));
		stats.fta = sqlite3_column_int(stmt, 20);
		stats.ft_pct = static_cast<float>(sqlite3_column_double(stmt, 21));
		stats.orb = sqlite3_column_int(stmt, 22);
		stats.drb = sqlite3_column_int(stmt, 23);
		stats.trb = sqlite3_column_int(stmt, 24);
		stats.ast = sqlite3_column_int(stmt, 25);
		stats.stl = sqlite3_column_int(stmt, 26);
		stats.blk = sqlite3_column_int(stmt, 27);
		stats.tov = sqlite3_column_int(stmt, 28);
		stats.pf = sqlite3_column_int(stmt, 29);
		stats.pts = sqlite3_column_int(stmt, 30);
		stats.awards =
			sqlite3_column_text(stmt, 31)
				? reinterpret_cast<const char *>(sqlite3_column_text(stmt, 31))
				: "";

		player_seasons.push_back(stats);
	}

	if (player_seasons.empty()) {
		error_msg = "No player found: " + player_name_input;
		std::cerr << "No results found" << std::endl;
	} else {
		std::cerr << "Found " << player_seasons.size() << " seasons for "
				  << player_name_input << std::endl;
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);
}

int main() {
	std::string player_name_input;
	std::string error_msg;
	int current_screen = 0;
	int selected_season = 0; // Index of currently selected season

	// Player Input
	InputOption input_option;
	input_option.multiline = false;
	input_option.on_enter = [&] {
		performSearch(player_name_input, error_msg);
		if (!error_msg.empty())
			current_screen = 0;
		else {
			current_screen = 1;
			selected_season = 0;
		}
	};
	auto input_player_name =
		Input(&player_name_input, "Enter Player Name", input_option);

	// League Toggle
	int toggle_selected = 0;
	std::vector<std::string> toggle_entries = {"NFL", "NBA", "NHL"};
	auto sport_toggle = Toggle(&toggle_entries, &toggle_selected);

	// Submit button
	auto submit_button = Button(
		"Search",
		[&] {
			if (!player_name_input.empty()) {
				performSearch(player_name_input, error_msg);
				if (!error_msg.empty())
					current_screen = 0;
				else {
					current_screen = 1;
					selected_season = 0;
				}
			}
		},
		Style());

	// Main screen back button
	auto back_button = Button("<-", [&] { current_screen = 0; }, Style());

	// Season Toggle
	std::vector<std::string> season_entries;
	auto season_toggle = Toggle(&season_entries, &selected_season);

	// Container - make sure all components are included
	auto component = Container::Vertical({
		sport_toggle,
		back_button,
		input_player_name,
		submit_button,
		season_toggle,
	});

	// Event Handler - update season entries after search
	auto event_handler = CatchEvent(component, [&](Event event) {
		if (event == Event::Return && input_player_name->Focused()) {
			performSearch(player_name_input, error_msg);
			if (!error_msg.empty()) {
				current_screen = 0;
			} else {
				current_screen = 1;
				player_name_input.clear();
				// Update season entries
				season_entries.clear();
				for (const auto &stats : player_seasons) {
					season_entries.push_back(stats.season);
				}
				selected_season = 0; // Reset to first season
			}
			return true;
		}
		// Handle toggle navigation (optional, for keyboard control)
		if (season_toggle->Focused()) {
			if (event == Event::ArrowLeft && selected_season > 0) {
				selected_season--;
				return true;
			}
			if (event == Event::ArrowRight &&
				selected_season < static_cast<int>(season_entries.size() - 1)) {
				selected_season++;
				return true;
			}
		}
		return false;
	});

	// Welcome Screen (unchanged)
	auto welcome = []() {
		return vbox({
				   text(L"   ______       __    ______              _          "
						L"__"),
				   text(L"  / __/ /____ _/ /_  /_  __/__ ______ _  (_)__  ___ "
						L"_/ /"),
				   text(L" _\\ \\/ __/ _ `/ __/   / / / -_) __/  ' \\/ / _ \\/ "
						L"_ `/ /"),
				   text(L"/___/\\__/\\_,_/\\__/   /_/  \\__/_/ "
						L"/_/_/_/_/_//_/\\_,_/_/"),
			   }) |
			   border;
	};

	// Player Info (unchanged)
	auto player_info = [&]() {
		if (player_seasons.empty()) {
			return vbox({text("No player data") | bold}) | border;
		}
		return vbox({
				   text("Player: " + player_seasons[0].player_name) | bold,
				   text("Latest Team: " + player_seasons[0].team),
				   text("Position: " + player_seasons[0].pos),
				   text("Latest Age: " + std::to_string(player_seasons[0].age)),
			   }) |
			   border;
	};

	// Stats Display - ensure it uses selected_season
	auto stats_display = [&]() {
		if (player_seasons.empty()) {
			if (!error_msg.empty()) {
				return text(error_msg) | color(Color::Red) | center;
			}
			return text("No player selected") | center;
		}

		// Bounds check to prevent out-of-range access
		if (selected_season < 0 ||
			selected_season >= static_cast<int>(player_seasons.size())) {
			return text("Invalid season selection") | center;
		}

		const auto &stats = player_seasons[selected_season];
		std::stringstream minutes, fg_pct, three_p_pct, two_p_pct, efg_pct,
			ft_pct;
		minutes << std::fixed << std::setprecision(2) << stats.mp;
		fg_pct << std::fixed << std::setprecision(2) << (stats.fg_pct * 100.0f);
		three_p_pct << std::fixed << std::setprecision(2)
					<< (stats.three_p_pct * 100.0f);
		two_p_pct << std::fixed << std::setprecision(2)
				  << (stats.two_p_pct * 100.0f);
		efg_pct << std::fixed << std::setprecision(2)
				<< (stats.efg_pct * 100.0f);
		ft_pct << std::fixed << std::setprecision(2) << (stats.ft_pct * 100.0f);

		return vbox({
				   text(stats.season) | bold,
				   separator(),
				   text("Team: " + stats.team),
				   text("Games: " + std::to_string(stats.g)),
				   text("Minutes: " + minutes.str()),
				   text("Points: " + std::to_string(stats.pts)),
				   text("FG: " + std::to_string(stats.fg) + "/" +
						std::to_string(stats.fga) + " (" + fg_pct.str() + "%)"),
				   text("3P: " + std::to_string(stats.three_p) + "/" +
						std::to_string(stats.three_pa) + " (" +
						three_p_pct.str() + "%)"),
				   text("FT: " + std::to_string(stats.ft) + "/" +
						std::to_string(stats.fta) + " (" + ft_pct.str() + "%)"),
				   text("Reb: " + std::to_string(stats.trb) +
						" (O:" + std::to_string(stats.orb) +
						", D:" + std::to_string(stats.drb) + ")"),
				   text("Ast: " + std::to_string(stats.ast)),
				   text("Stl: " + std::to_string(stats.stl)),
				   text("Blk: " + std::to_string(stats.blk)),
				   text("TO: " + std::to_string(stats.tov)),
				   text("PF: " + std::to_string(stats.pf)),
				   text("Awards: " + stats.awards),
			   }) |
			   border | flex_grow;
	};

	// Renderer - ensure toggle is interactive
	auto renderer = Renderer(component, [&]() -> Element {
		if (current_screen == 0) {
			return vbox({
					   welcome(),
					   hbox({text("Select a league: "),
							 sport_toggle->Render()}) |
						   hcenter | vcenter | border | xflex,
					   hbox({text("Player: ") | vcenter,
							 input_player_name->Render() | vcenter |
								 size(WIDTH, GREATER_THAN, 20),
							 submit_button->Render()}) |
						   hcenter | border,
					   error_msg.empty()
						   ? text("")
						   : text(error_msg) | color(Color::Red) | center,
				   }) |
				   vcenter | hcenter | border;
		} else {
			return vbox({
					   hbox({
						   back_button->Render(),
						   input_player_name->Render() | xflex_grow,
						   submit_button->Render(),
						   season_toggle->Render() |
							   size(WIDTH, GREATER_THAN,
									20), // Removed focusable
					   }) | border,
					   hbox({
						   player_info() | size(WIDTH, EQUAL, 22),
						   stats_display() | flex_grow,
					   }) | flex_grow,
				   }) |
				   size(HEIGHT, GREATER_THAN, 40) | border;
		}
	});

	auto screen = ScreenInteractive::Fullscreen();
	screen.Loop(renderer);
	return 0;
}
