// main.cpp - 上海地铁路径规划与管理系统主程序入口
#include "graph.h"
#include "menu.h"
#include "pathfinder.h"
#include "station.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

const std::string STATION_CSV = "data/Station.csv";
const std::string STATION_INIT = "data/Station_init.csv";
const std::string EDGE_CSV = "data/Edge.csv";
const std::string UPDATE_CSV = "data/update_station_status.csv";

// 程序主入口：解析参数，加载数据，启动菜单
int main(int argc, char **argv) {
#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif

  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--help" || a == "-h" || a == "/?") {
      std::cout << "用法:\n"
                << "  metro.exe              进入交互式菜单\n"
                << "  metro.exe --help       显示本帮助\n";
      return 0;
    }
  }

  std::cout << "========================================\n";
  std::cout << "  上海地铁路径规划与管理系统 启动中...\n";
  std::cout << "========================================\n";

  StationManager sm;
  if (!sm.loadFromCSV(STATION_CSV)) {
    std::cerr << "[错误] 加载 " << STATION_CSV << " 失败！\n";
    return 1;
  }
  std::cout << "[OK] 加载站点: " << sm.allStations().size() << " 个\n";

  if (!sm.loadInitFromCSV(STATION_INIT)) {
    std::cerr << "[警告] 加载 " << STATION_INIT << " 失败，"
              << "一键恢复功能将不可用。\n";
  } else {
    std::cout << "[OK] 加载初始状态快照\n";
  }

  Graph graph(sm);
  if (!graph.build(EDGE_CSV)) {
    std::cerr << "[错误] 构建图失败！\n";
    return 1;
  }
  std::cout << "[OK] 图构建完成，共 " << graph.nodeCount() << " 个顶点\n";

  PathFinder pf(graph, sm);

  Menu menu(sm, graph, pf);
  menu.run();

  std::cout << "\n========================================\n";
  std::cout << "  感谢使用, 再见!\n";
  std::cout << "  >>> 按回车键退出程序...\n";
  std::cout << "========================================\n";
#ifdef _WIN32
  std::cout.flush();
#endif
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  std::cin.get();
  return 0;
}

// Unity Build: 把所有 .cpp 直接 include 进来，使 g++ main.cpp 一次编译即可
#ifndef NO_UNITY_BUILD
#include "graph.cpp"
#include "menu.cpp"
#include "pathfinder.cpp"
#include "station.cpp"
#endif
