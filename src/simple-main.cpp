#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "sqlite3.h"
#include <iostream>
#include <string>

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

int main() {
	std::string player_name_input;
	PlayerStats current_stats;
	std::string error_msg;

	// Input field
	auto input = Input(&player_name_input, "Enter player name");

	// Search button
	auto button = Button("Search", [&] {
		std::cerr << "Searching for: " << player_name_input << std::endl;
		error_msg.clear();
		current_stats = PlayerStats{}; // Reset stats

		sqlite3 *db;
		int rc = sqlite3_open("stat-term.db", &db);
		if (rc != SQLITE_OK) {
			error_msg =
				"Can't open database: " + std::string(sqlite3_errmsg(db));
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

		sqlite3_bind_text(stmt, 1, player_name_input.c_str(), -1,
						  SQLITE_STATIC);
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
	});

	// Layout: input and button on top, stats below
	auto component = Container::Horizontal({input, button});
	auto renderer = Renderer(component, [&] {
		if (player_name_input.empty() && current_stats.name.empty() &&
			error_msg.empty()) {
			return vbox({
					   text("Type a name and click Search"),
					   hbox({input->Render(), button->Render()}),
				   }) |
				   border;
		} else if (!error_msg.empty()) {
			return vbox({
					   hbox({input->Render(), button->Render()}),
					   text(error_msg) | color(Color::Red),
				   }) |
				   border;
		} else {
			return vbox({
					   hbox({input->Render(), button->Render()}),
					   text("Player: " + current_stats.name) | bold,
					   text("Team: " + current_stats.team),
					   text("Position: " + current_stats.position),
					   text("Age: " + std::to_string(current_stats.age)),
					   text("Height: " + current_stats.height),
					   text("Weight: " + std::to_string(current_stats.weight) +
							" lbs"),
					   text("Passing Yards: " +
							std::to_string(current_stats.passing_yards)),
					   text("Touchdowns: " +
							std::to_string(current_stats.touchdowns)),
					   text("Interceptions: " +
							std::to_string(current_stats.interceptions)),
					   text("Completion %: " +
							std::to_string(current_stats.completion_pct) + "%"),
					   text("Rushing Yards: " +
							std::to_string(current_stats.rushing_yards)),
					   text("Games Played: " +
							std::to_string(current_stats.games_played)),
				   }) |
				   border;
		}
	} Style());

	// Initialize database with full dummy data
	sqlite3 *db;
	int rc = sqlite3_open("stat-term.db", &db);
	if (rc == SQLITE_OK) {
		const char *create_table =
			"CREATE TABLE IF NOT EXISTS nfl_player_stats ("
			"player_name TEXT, team TEXT, position TEXT, age INTEGER, height "
			"TEXT, weight INTEGER, "
			"passing_yards INTEGER, touchdowns INTEGER, interceptions INTEGER, "
			"completion_pct REAL, "
			"rushing_yards INTEGER, games_played INTEGER)";
		sqlite3_exec(db, create_table, nullptr, nullptr, nullptr);

		const char *insert_data =
			"INSERT OR IGNORE INTO nfl_player_stats VALUES "
			"('Jalen Hurts', 'Philadelphia Eagles', 'QB', 26, '6''1\"', 223, "
			"3850, 28, 9, 67.8, 650, 16),"
			"('C.J. Stroud', 'Houston Texans', 'QB', 23, '6''3\"', 218, 4200, "
			"32, 10, 69.2, 420, 16)";
		char *err;
		rc = sqlite3_exec(db, insert_data, nullptr, nullptr, &err);
		if (rc != SQLITE_OK) {
			std::cerr << "Insert failed: " << err << std::endl;
			sqlite3_free(err);
		} else {
			std::cerr << "Dummy data inserted successfully" << std::endl;
		}
		sqlite3_close(db);
	} else {
		std::cerr << "Failed to open DB for init: " << sqlite3_errmsg(db)
				  << std::endl;
	}

	auto screen = ScreenInteractive::Fullscreen();
	screen.Loop(renderer);

	return 0;
}
