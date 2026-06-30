// 控制台菜单与交互流程。
#pragma once

#include "pathfinder.h"
#include <string>

class Menu {
public:
  Menu(StationManager &stationManager, Graph &graph, PathFinder &pathFinder)
      : stationManager_(stationManager), graph_(graph), pathFinder_(pathFinder) {}

  void run();

private:
  StationManager &stationManager_;
  Graph &graph_;
  PathFinder &pathFinder_;

  static void clearScreen();
  static void pause();
  static int readInt(const std::string &prompt, int defaultVal = -1);
  static std::string readLine(const std::string &prompt);

  int fuzzyPickStation(const std::string &prompt);

  void stationMenu();
  void timePathMenu();
  void transferPathMenu();
  void impactMenu();

  void showClosedStations();
  void showLineStations();
  void toggleOneStation();
  void batchUpdateFromCSV();
  void restoreInitial();
  void runPathQuery(int mode);

  void addNewStation();
  void removeExistingStation();
  void addNewEdgeInteractive();
};
