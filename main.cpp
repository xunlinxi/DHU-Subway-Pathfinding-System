// =============================================================
//  main.cpp
//  上海地铁路径规划与管理系统 —— 主程序入口
// =============================================================
#include "graph.h"
#include "menu.h"
#include "pathfinder.h"
#include "station.h"
#include <cstdlib>
#include <iostream>
#include <string>


#ifdef _WIN32
#include <windows.h>
#endif

// 数据文件路径（按 §3.5 项目结构放在 data/ 子目录下）
const std::string STATION_CSV   = "data/Station.csv";
const std::string STATION_INIT  = "data/Station_init.csv";
const std::string EDGE_CSV      = "data/Edge.csv";
const std::string UPDATE_CSV    = "data/update_station_status.csv";

int main() {
#ifdef _WIN32
  // 让 Windows 控制台支持 UTF-8 中文输出
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif

  std::cout << "========================================\n";
  std::cout << "  上海地铁路径规划与管理系统 启动中...\n";
  std::cout << "========================================\n";

  // 1) 加载站点
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

  // 2) 构建图
  Graph graph(sm);
  if (!graph.build(EDGE_CSV)) {
    std::cerr << "[错误] 构建图失败！\n";
    return 1;
  }
  std::cout << "[OK] 图构建完成，共 " << graph.nodeCount() << " 个顶点\n";

  // 3) 路径查找器
  PathFinder pf(graph, sm);

  // 4) 启动菜单
  Menu menu(sm, graph, pf);
  menu.run();
  return 0;
}
