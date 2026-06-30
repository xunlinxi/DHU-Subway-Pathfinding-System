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

  // ---------- 菜单 ----------
  void mainMenu();
  void stationMenu();    // 站点管理
  void timeMenu();       // 所需时间最短路径规划（独立子菜单）
  void transferMenu();   // 所需换乘次数最少路径规划（独立子菜单）
  void buildMenu();      // §3.3 建站管理（可选加分）

  // ---------- 业务 ----------
  void showClosedStations();
  void showLineStations();
  void queryStations();
  void toggleOneStation();
  void batchUpdateFromCSV();
  void restoreInitial();
  void runImpactAnalysis();
  void runPathQuery(int mode);  // 0=time, 1=transfer, 2=K-time, 3=K-transfer
  void saveData();

  // ---- 运营管理扩展 ----
  void closeTransferStationMenu();   // 换乘站整体关闭
  void lineOutageMenu();             // 线路停运管理
  void networkOutageMenu();          // 全网停运/恢复
  void networkConnectivityMenu();    // 网络连通性分析
};
