// =============================================================
//  menu.h
//  控制台菜单交互：站点管理、路径查询、影响分析
//  - 二级菜单
//  - 模糊搜索 + 换乘站二次选择
//  - 统一调用 StationManager / Graph / PathFinder
// =============================================================
#pragma once

#include "pathfinder.h"
#include <string>
#include <vector>

class Menu {
public:
  Menu(StationManager &sm, Graph &g, PathFinder &pf)
      : sm_(sm), graph_(g), pf_(pf) {}

  // 主循环入口
  void run();

private:
  StationManager &sm_;
  Graph &graph_;
  PathFinder &pf_;

  // ---------- 通用 UI ----------
  static void clearScreen();
  static void pause();
  static int readInt(const std::string &prompt, int defaultVal = -1);
  static std::string readLine(const std::string &prompt);
  static void printHeader(const std::string &title);
  static void printStations(const std::vector<Station> &sts);

  // 模糊选择站点：返回选中的 station.id；用户取消返回 -1
  int fuzzyPickStation(const std::string &prompt);

  // ---------- 菜单 ----------
    void mainMenu();
    void stationMenu();   // 站点管理
    void pathMenu();      // 路径查询
    void impactMenu();    // 受影响分析
    void buildMenu();     // §3.3 建站管理（可选加分）

    // ---------- 业务 ----------
    void showClosedStations();
    void showLineStations();
    void toggleOneStation();
    void batchUpdateFromCSV();
    void restoreInitial();
    void runImpactAnalysis();
    void runPathQuery(int mode);  // 0=time, 1=transfer, 2=K-time, 3=K-transfer
    void saveData();

    // §3.3 建站管理
    void addNewStation();
    void removeExistingStation();
    void addNewEdgeInteractive();
};
