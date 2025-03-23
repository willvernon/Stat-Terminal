# Stats Terminal
A TUI App

Welcome to the Sports Stats Terminal App! This is a TUI designed to provide fast and easy access to player statistics for various sports, with a current focus on basketball. Built for sports enthusiasts, this app leverages a sleek terminal interface powered by FTXUI and stores data locally using SQLite.
### Features

- Basketball Player Stats: Retrieve detailed stats for NBA players (e.g., points, rebounds, assists, etc.).
- Interactive UI: Enjoy a user-friendly terminal interface powered by FTXUI.
- Local Database: Stats are stored in an SQLite database for quick access.
- Future-Ready: Designed to expand to other sports (e.g., football, baseball) in later updates.

### Technologies

- FTXUI: A C++ library for building interactive terminal user interfaces.
- SQLite: A lightweight, embedded database for storing and querying player stats.

#### Splash Screen:
![CleanShot 2025-03-23 at 18 26 08](https://github.com/user-attachments/assets/092118a1-d61a-4e8a-933c-4719e9ab374c)

#### Main Screen:
![CleanShot 2025-03-23 at 18 26 38](https://github.com/user-attachments/assets/43011f81-1028-4e13-8a3d-af012b5d528c)

### Installation

Clone the Repository

git clone 
cd stat-terminal
Install Dependencies
Ensure you have the following installed:

- A C++ compiler (e.g., g++)
- FTXUI (follow their installation guide)
- SQLite3 (sudo apt install libsqlite3-dev on Ubuntu, or equivalent for your OS)

### Build the App
```bash
mkdir build
# make sure to copy the db to the build file
cp -riv ~Path-To/Stat-Terminal/db/stat-term.db ~Path-To/Stat-Terminal/build/
cd build
cmake ..
make
./stat-terminal
```
### Database Setup
Note: The SQLite database file is not included in this repository at the moment. To use the app:

- Create your own SQLite database with the required schema (details TBD in future updates).
- Place the .db file in the appropriate directory (e.g., ../db/stat-term.db).
- Update the app configuration to point to your database file if needed.

Run the App
```bash
./sports-stats
```
### Usage

- Launch the app in your terminal.
- Use the FTXUI interface to search for players or browse stats.
- Example commands (to be implemented):
    - search LeBron James - Capilization Matters - Displays LeBron's latest stats.

### Current Status

- Focused on basketball stats.
- Database schema and sample data are not yet included in the repo (coming soon!).
- Additional sports support is in the planning stages.

TODO
<!--TODO-->

- [x] get interface to work with data
- [x] connect DB to the interface
- [ ] Get a full DB of NFL idv player stats
- [ ] Clean up the UI

  - [ ] Add a clear or reset button
  - [ ] Add a season selector drop down
  - [ ] Add a team selector on the splash
  - [ ]

- [?] NLP for searching for stats
- [?]

