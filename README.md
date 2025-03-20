# Stat Terminal

=======

# Todo

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

# Build instructions:

```bash
mkdir build
# make sure to copy the db to the build file
cp -riv ~Path-To/Stat-Terminal/db/stat-term.db ~Path-To/Stat-Terminal/build/
cd build
cmake ..
make
./stat-terminal
```
