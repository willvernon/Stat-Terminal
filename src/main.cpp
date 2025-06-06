#include "ftxui/component/captured_mouse.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/util/ref.hpp"
#include "sqlite3.h"
#include <chrono>
#include <cstdlib> // For getenv
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace ftxui;

// Define PlayerStats struct first
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

// Global variables
PlayerStats current_stats;
bool show_copy_dialog = false;

// Style function
ButtonOption Style() {
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

// Search function
void performSearch(const std::string &player_name_input,
				   std::string &error_msg) {
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
		"fg, fga, fg_pct, three_p, three_pa, three_p_pct, two_p, two_pa, "
		"two_p_pct, efg_pct, ft, fta, ft_pct, orb, drb, trb, ast, stl, blk, "
		"tov, pf, pts, awards "
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
		current_stats.season =
			sqlite3_column_text(stmt, 0)
				? reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0))
				: "";
		current_stats.rk = sqlite3_column_int(stmt, 1);
		current_stats.player_name =
			sqlite3_column_text(stmt, 2)
				? reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2))
				: "";
		current_stats.age = sqlite3_column_int(stmt, 3);
		current_stats.team =
			sqlite3_column_text(stmt, 4)
				? reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4))
				: "";
		current_stats.pos =
			sqlite3_column_text(stmt, 5)
				? reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5))
				: "";
		current_stats.g = sqlite3_column_int(stmt, 6);
		current_stats.gs = sqlite3_column_int(stmt, 7);
		current_stats.mp = static_cast<float>(sqlite3_column_double(stmt, 8));
		current_stats.fg = sqlite3_column_int(stmt, 9);
		current_stats.fga = sqlite3_column_int(stmt, 10);
		current_stats.fg_pct =
			static_cast<float>(sqlite3_column_double(stmt, 11));
		current_stats.three_p = sqlite3_column_int(stmt, 12);
		current_stats.three_pa = sqlite3_column_int(stmt, 13);
		current_stats.three_p_pct =
			static_cast<float>(sqlite3_column_double(stmt, 14));
		current_stats.two_p = sqlite3_column_int(stmt, 15);
		current_stats.two_pa = sqlite3_column_int(stmt, 16);
		current_stats.two_p_pct =
			static_cast<float>(sqlite3_column_double(stmt, 17));
		current_stats.efg_pct =
			static_cast<float>(sqlite3_column_double(stmt, 18));
		current_stats.ft = static_cast<float>(sqlite3_column_double(stmt, 19));
		current_stats.fta = sqlite3_column_int(stmt, 20);
		current_stats.ft_pct =
			static_cast<float>(sqlite3_column_double(stmt, 21));
		current_stats.orb = sqlite3_column_int(stmt, 22);
		current_stats.drb = sqlite3_column_int(stmt, 23);
		current_stats.trb = sqlite3_column_int(stmt, 24);
		current_stats.ast = sqlite3_column_int(stmt, 25);
		current_stats.stl = sqlite3_column_int(stmt, 26);
		current_stats.blk = sqlite3_column_int(stmt, 27);
		current_stats.tov = sqlite3_column_int(stmt, 28);
		current_stats.pf = sqlite3_column_int(stmt, 29);
		current_stats.pts = sqlite3_column_int(stmt, 30);
		current_stats.awards =
			sqlite3_column_text(stmt, 31)
				? reinterpret_cast<const char *>(sqlite3_column_text(stmt, 31))
				: "";
	} else {
		error_msg = "No player found: " + player_name_input;
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);
}

// Export CSV function
bool exportStatsToCSV() {
	const char *home_dir = std::getenv("HOME");
	if (!home_dir) {
		std::cerr << "Could not get HOME environment variable" << std::endl;
		return false;
	}

	std::string full_path =
		std::string(home_dir) + "/Downloads/player_stats.csv";
	std::ofstream file(full_path);
	if (!file.is_open()) {
		std::cerr << "Failed to open file at " << full_path << std::endl;
		return false;
	}

	file << "Season,Rk,Player,Age,Team,Position,Games,Games Started,Minutes,"
		 << "FG,FGA,FG%,3P,3PA,3P%,2P,2PA,2P%,eFG%,FT,FTA,FT%,ORB,DRB,TRB,"
		 << "AST,STL,BLK,TOV,PF,PTS,Awards\n";

	file << current_stats.season << "," << current_stats.rk << ","
		 << current_stats.player_name << "," << current_stats.age << ","
		 << current_stats.team << "," << current_stats.pos << ","
		 << current_stats.g << "," << current_stats.gs << ","
		 << current_stats.mp << "," << current_stats.fg << ","
		 << current_stats.fga << "," << current_stats.fg_pct << ","
		 << current_stats.three_p << "," << current_stats.three_pa << ","
		 << current_stats.three_p_pct << "," << current_stats.two_p << ","
		 << current_stats.two_pa << "," << current_stats.two_p_pct << ","
		 << current_stats.efg_pct << "," << current_stats.ft << ","
		 << current_stats.fta << "," << current_stats.ft_pct << ","
		 << current_stats.orb << "," << current_stats.drb << ","
		 << current_stats.trb << "," << current_stats.ast << ","
		 << current_stats.stl << "," << current_stats.blk << ","
		 << current_stats.tov << "," << current_stats.pf << ","
		 << current_stats.pts << "," << current_stats.awards << "\n";

	file.close();
	return true;
}

int main() {
	std::string player_name_input;
	std::string error_msg;
	int current_screen = 0;

	auto screen = ScreenInteractive::Fullscreen();

	InputOption input_option;
	input_option.on_enter = [&] {
		performSearch(player_name_input, error_msg);
		if (!error_msg.empty())
			current_screen = 0;
		else
			current_screen = 1;
	};
	auto input_player_name =
		Input(&player_name_input, "Enter Player Name", input_option);

	int toggle_selected = 0;
	std::vector<std::string> toggle_entries = {"NFL", "NBA", "NHL"};
	auto sport_toggle = Toggle(&toggle_entries, &toggle_selected);

	auto submit_button = Button(
		"Search",
		[&] {
			if (!player_name_input.empty()) {
				performSearch(player_name_input, error_msg);
				if (!error_msg.empty())
					current_screen = 0;
				else
					current_screen = 1;
			}
		},
		Style());

	auto back_button = Button("<-", [&] { current_screen = 0; }, Style());

	auto export_button = Button(
		"Export CSV",
		[&] {
			if (!current_stats.player_name.empty() && exportStatsToCSV()) {
				show_copy_dialog = true;
				std::thread([&] {
					std::this_thread::sleep_for(std::chrono::seconds(2));
					show_copy_dialog = false;
					screen.PostEvent(Event::Custom);
				}).detach();
			}
		},
		Style());

	auto component = Container::Vertical({
		sport_toggle,
		input_player_name,
		submit_button,
		back_button,
		export_button,
	});

	auto event_handler = CatchEvent(component, [&](Event event) {
		if (event == Event::Return && input_player_name->Focused()) {
			performSearch(player_name_input, error_msg);
			if (!error_msg.empty())
				current_screen = 0;
			else
				current_screen = 1;
			player_name_input.clear();
			return true;
		}
		return false;
	});

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

	auto player_info = [&]() {
		std::vector<Element> elements = {
			vbox({
				text("Player: " + current_stats.player_name) | bold,
				text("Team: " + current_stats.team),
				text("Position: " + current_stats.pos),
				text("Age: " + std::to_string(current_stats.age)),
			}) |
			border};

		elements.push_back(export_button->Render() | border);
		if (show_copy_dialog) {
			elements.push_back(
				vbox({filler(),
					  hbox({filler(),
							vbox({text("Saved to Downloads") | center}) |
								border | size(WIDTH, EQUAL, 40),
							filler()}),
					  filler()}) |
				vcenter | hcenter);
		}

		return vbox(elements);
	};

	auto stats_display = [&]() {
		if (current_stats.player_name.empty()) {
			if (!error_msg.empty()) {
				return text(error_msg) | color(Color::Red) | center;
			}
			return text("No player selected") | center;
		}

		std::stringstream minutes, ftm, fg_pct, three_p_pct, two_p_pct, efg_pct,
			ft_pct;
		fg_pct << std::fixed << std::setprecision(2)
			   << (current_stats.fg_pct * 100.0f);
		three_p_pct << std::fixed << std::setprecision(2)
					<< (current_stats.three_p_pct * 100.0f);
		two_p_pct << std::fixed << std::setprecision(2)
				  << (current_stats.two_p_pct * 100.0f);
		efg_pct << std::fixed << std::setprecision(2)
				<< (current_stats.efg_pct * 100.0f);
		ft_pct << std::fixed << std::setprecision(2)
			   << (current_stats.ft_pct * 100.0f);

		return vbox({
				   text("Player Stats - " + current_stats.season) | bold,
				   separator(),
				   hbox({text("Games: " + std::to_string(current_stats.g))}),
				   hbox({text("Minutes: " + std::to_string(current_stats.mp))}),
				   hbox({text("Points: " + std::to_string(current_stats.pts))}),
				   hbox({text("FG: " + std::to_string(current_stats.fg) + "/" +
							  std::to_string(current_stats.fga) + " (" +
							  fg_pct.str() + "%)")}),
				   hbox({text("3P: " + std::to_string(current_stats.three_p) +
							  "/" + std::to_string(current_stats.three_pa) +
							  " (" + three_p_pct.str() + "%)")}),
				   hbox({text("FT: " + std::to_string(current_stats.ft) + "/" +
							  std::to_string(current_stats.fta) + " (" +
							  ft_pct.str() + "%)")}),
				   hbox({text("Rebounds: " + std::to_string(current_stats.trb) +
							  " (O: " + std::to_string(current_stats.orb) +
							  ", D: " + std::to_string(current_stats.drb) +
							  ")")}),
				   hbox(
					   {text("Assists: " + std::to_string(current_stats.ast))}),
				   hbox({text("Steals: " + std::to_string(current_stats.stl))}),
				   hbox({text("Blocks: " + std::to_string(current_stats.blk))}),
				   hbox({text("Turnovers: " +
							  std::to_string(current_stats.tov))}),
				   hbox({text("Fouls: " + std::to_string(current_stats.pf))}),
				   hbox({text("Awards: " + current_stats.awards)}),
			   }) |
			   border | flex;
	};

	auto renderer = Renderer(event_handler, [&]() -> Element {
		Element content;
		if (current_screen == 0) {
			content =
				vbox({
					welcome(),
					hbox({text("Select a league: "), sport_toggle->Render()}) |
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
			content =
				gridbox({
					{
						vbox({player_info() | size(HEIGHT, GREATER_THAN, 10)}) |
							size(WIDTH, EQUAL, 22),
						vbox({
							hbox({
								back_button->Render(),
								input_player_name->Render() | vcenter |
									xflex_grow,
								submit_button->Render(),
							}) | border,
							stats_display() | flex_grow,
						}) | flex_grow,
					},
				}) |
				size(HEIGHT, GREATER_THAN, 70) | border;
		}

		return vbox({filler(), hbox({filler(), content | flex, filler()}),
					 filler()}) |
			   vcenter | hcenter;
	});

	screen.Loop(renderer);
	return 0;
}
