#include "station.h"
#include <iostream>
int main() {
  StationManager sm;
  if (!sm.loadFromCSV("data/Station.csv")) return 1;
  auto hits = sm.fuzzySearch("×í");
  for (const auto &s : hits) {
    std::cout << s.name << " (" << s.line << ")\n";
  }
  return 0;
}
