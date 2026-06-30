// menu.h - 控制台菜单与交互流程
#pragma once

#include "pathfinder.h"
#include <string>

// 主菜单控制器：站点管理、路径查询、运营管理
class Menu {
public:
  Menu(StationManager &stationManager, Graph &graph, PathFinder &pathFinder)
      : stationManager_(stationManager), graph_(graph),
        pathFinder_(pathFinder) {}

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

  void mainMenu();
  void stationMenu();
  void timeMenu();
  void transferMenu();
  void buildMenu();

  void showClosedStations();
  void showLineStations();
  void queryStations();
  void toggleOneStation();
  void batchUpdateFromCSV();
  void restoreInitial();
  void runImpactAnalysis();
  void runPathQuery(int mode);

  // 运营管理扩展
  void closeTransferStationMenu();
  void lineOutageMenu();
  void networkOutageMenu();
  void networkConnectivityMenu();
};
