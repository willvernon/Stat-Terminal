#include "ftxui/component/captured_mouse.hpp" // for ftxui
#include "ftxui/component/component.hpp" // for Input, Renderer, Vertical, Dropdown
#include "ftxui/component/component_base.hpp"	 // for ComponentBase
#include "ftxui/component/component_options.hpp" // for InputOption
#include "ftxui/component/screen_interactive.hpp" // for Component, ScreenInteractive
#include "ftxui/dom/elements.hpp" // for text, hbox, separator, etc.
#include "ftxui/util/ref.hpp"	  // for Ref
#include "sqlite3.h"			  // SQLite C API
#include <iostream>				  // for std::cout, std::endl
#include <memory>				  // for allocator, __shared_ptr_access
#include <string>				  // for string
#include <vector>				  // for std::vector

using namespace ftxui;

// Struct to hold player stats
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

// Button Styles
ButtonOption Style() {
	auto option = ButtonOption::Animated();
	option.transform = [](const EntryState &s) {
		auto element = text(s.label);
		if (s.focused) {
			element |= bold;
		}
		return element | center | size(WIDTH, EQUAL, 15) |
			   size(HEIGHT, EQUAL, 1);
	};
	return option;
}

// Function to fetch all player names from the database Really only needed for
// dropdown
// TODO: May Delete fetching all player names dont need unless using dropdown
std::vector<std::string> fetchPlayerNames() {
	std::vector<std::string> player_names;
	sqlite3 *db;
	int rc = sqlite3_open("stat-term.db", &db);
	if (rc != SQLITE_OK) {
		std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
		sqlite3_close(db);
		return player_names;
	}

	sqlite3_stmt *stmt;
	// Use DISTINCT to avoid duplicates
	const char *query = "SELECT DISTINCT player_name FROM nfl_player_stats "
						"ORDER BY player_name";
	rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		std::cerr << "SQL error in prepare: " << sqlite3_errmsg(db)
				  << std::endl;
		sqlite3_close(db);
		return player_names;
	}

	std::cerr << "Fetching player names from database..." << std::endl;
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		const char *name =
			reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
		player_names.push_back(name ? name : ""); // Handle NULL values
		std::cerr << "Fetched: " << player_names.back() << std::endl;
	}
	if (rc != SQLITE_DONE) {
		std::cerr << "Error stepping through results: " << sqlite3_errmsg(db)
				  << std::endl;
	}

	std::cerr << "Total unique players fetched: " << player_names.size()
			  << std::endl;
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return player_names;
}

// Function to perform the search
void performSearch(const std::string &player_name_input,
				   PlayerStats &current_stats, std::string &error_msg) {
	std::cerr << "Searching for: " << player_name_input << std::endl;
	error_msg.clear();
	current_stats = PlayerStats{}; // Reset stats

	sqlite3 *db;
	int rc = sqlite3_open("stat-term.db", &db);
	if (rc != SQLITE_OK) {
		error_msg = "Can't open database: " + std::string(sqlite3_errmsg(db));
		std::cerr << error_msg << std::endl;
		sqlite3_close(db);
		return;
	}

	sqlite3_stmt *stmt;
	const char *query =
		"SELECT player_name, team, position, age, height, weight, "
		"passing_yards, touchdowns, interceptions, completion_pct, "
		"rushing_yards, games_played FROM nfl_player_stats WHERE "
		"player_name = ?";
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
		current_stats.name =
			reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
		current_stats.team =
			reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
		current_stats.position =
			reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
		current_stats.age = sqlite3_column_int(stmt, 3);
		current_stats.height =
			reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
		current_stats.weight = sqlite3_column_int(stmt, 5);
		current_stats.passing_yards = sqlite3_column_int(stmt, 6);
		current_stats.touchdowns = sqlite3_column_int(stmt, 7);
		current_stats.interceptions = sqlite3_column_int(stmt, 8);
		current_stats.completion_pct =
			static_cast<float>(sqlite3_column_double(stmt, 9));
		current_stats.rushing_yards = sqlite3_column_int(stmt, 10);
		current_stats.games_played = sqlite3_column_int(stmt, 11);
		std::cerr << "Found: " << current_stats.name << ", "
				  << current_stats.team << std::endl;
	} else {
		error_msg = "No player found: " + player_name_input;
		std::cerr << "Step returned: " << rc << " (100=ROW, 101=DONE)"
				  << std::endl;
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);
}

int main() {
	// FTXUI UI setup
	std::string player_name_input; // Input buffer for typing
	PlayerStats current_stats;	   // Holds the result of the search
	std::string error_msg;
	std::vector<std::string> player_names =
		fetchPlayerNames();		 // Fetch all player names
	int selected_player_idx = 0; // Index of selected player in dropdown

	// Input component
	InputOption input_option;
	input_option.on_enter = [&] {
		performSearch(player_name_input, current_stats, error_msg);
	};
	auto input_player_name =
		Input(&player_name_input, "Enter player name", input_option);

	// Dropdown component
	/*auto dropdown = Dropdown(&player_names, &selected_player_idx);*/

	// State to track current screen (0 = splash, 1 = main page)
	int current_screen = 0;

	// Back button
	auto back_button = Button(
		"<-",
		[&] {
			current_screen = 0; // Back to splash page
		},
		Style());

	// Search button
	auto submit_button = Button(
		"Search",
		[&] {
			if (!player_name_input.empty()) {
				current_screen = 1;
				performSearch(player_name_input, current_stats, error_msg);
			} else if (!player_names.empty() && selected_player_idx >= 0) {
				current_screen = 0;
				performSearch(player_names[selected_player_idx], current_stats,
							  error_msg);
			}
		},
		Style());

	// Container to manage components
	auto component =
		Container::Horizontal({input_player_name, submit_button, back_button});

	// Add custom event handling for Enter key
	auto event_handler = CatchEvent(component, [&](Event event) {
		if (event == Event::Return && input_player_name->Focused()) {
			performSearch(player_name_input, current_stats, error_msg);
			current_screen = 1;
			player_name_input.clear();

			return true; // Event handled
		}
		return false; // Event not handled
	});

	// Welcome Logo (Splash Screen)
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

	// Player Info Panel (left col)
	auto player_info = [&]() {
		return vbox({
				   text("Player: " + current_stats.name) | bold,
				   text("Team: " + current_stats.team),
				   text("Position: " + current_stats.position),
				   text("Age: " + std::to_string(current_stats.age)),
				   text("Height: " + current_stats.height),
				   text("Weight: " + std::to_string(current_stats.weight) +
						" lbs"),
			   }) |
			   border;
	};

	// Player Stats (Main Display)
	auto stats_display = [&]() {
		if (current_stats.name.empty()) {
			if (!error_msg.empty()) {
				return text(error_msg) | color(Color::Red) | center;
			}
			return text("No player selected") | center;
		}
		return vbox({
				   text("Player Stats") | bold,
				   separator(),
				   hbox({text("Passing Yards: " +
							  std::to_string(current_stats.passing_yards))}),
				   hbox({text("Touchdowns: " +
							  std::to_string(current_stats.touchdowns))}),
				   hbox({text("Interceptions: " +
							  std::to_string(current_stats.interceptions))}),
				   hbox({text("Completion %: " +
							  std::to_string(current_stats.completion_pct) +
							  "%")}),
				   hbox({text("Rushing Yards: " +
							  std::to_string(current_stats.rushing_yards))}),
				   hbox({text("Games Played: " +
							  std::to_string(current_stats.games_played))}),
			   }) |
			   border | flex;
	};

	// Renderer
	auto renderer = Renderer(event_handler, [&] {
		// Stay on splash screen until a successful search is performed
		if (current_screen == 0) {
			// Splash Screen
			return vbox({
					   welcome(),
					   hbox({
						   text("Player: ") | vcenter,
						   input_player_name->Render() | vcenter |
							   size(WIDTH, GREATER_THAN, 20),
						   /*dropdown->Render() | vcenter,*/
						   submit_button->Render(),
					   }) | hcenter |
						   border,
					   error_msg.empty()
						   ? text("")
						   : text(error_msg) | color(Color::Red) | center,
				   }) |
				   vcenter | hcenter | border;
		} else {
			// Stat Screen
			return gridbox({
					   {
						   vbox({player_info() |
								 size(HEIGHT, GREATER_THAN, 10)}) |
							   size(WIDTH, EQUAL, 22),
						   vbox({
							   hbox({
								   back_button->Render(),
								   text("Search New Player: "),
								   input_player_name->Render() | vcenter | flex,
								   /*dropdown->Render() | vcenter,*/
								   submit_button->Render(),
							   }) | border,
							   stats_display() | flex_grow,
						   }) | flex_grow,
					   },
				   }) |
				   size(HEIGHT, GREATER_THAN, 40) | border;
		}
	});

	auto screen = ScreenInteractive::Fullscreen();
	screen.Loop(renderer);

	return 0;
}
