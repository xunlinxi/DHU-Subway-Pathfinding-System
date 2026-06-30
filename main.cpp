// =============================================================
//  main.cpp
//  上海地铁路径规划与管理系统 —— 主程序入口
//
//  使用方式:
//    1) 交互式菜单 (默认):
//         g++ -std=c++11 -O2 -o metro.exe main.cpp
//         .\metro.exe
//
//    2) 一键演示模式 (跑预设测试用例, 不需要交互):
//         .\metro.exe --demo
//
//    3) 传统多文件编译 (如不需要 unity build):
//         g++ -std=c++11 -O2 -DNO_UNITY_BUILD -o metro.exe main.cpp station.cpp graph.cpp pathfinder.cpp menu.cpp
//         .\metro.exe
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
const std::string STATION_CSV  = "data/Station.csv";
const std::string STATION_INIT = "data/Station_init.csv";
const std::string EDGE_CSV     = "data/Edge.csv";
const std::string UPDATE_CSV   = "data/update_station_status.csv";

// =====================================================================
//   一键演示模式：跑一组预设测试用例，覆盖核心功能
//   用于：① 验收 ② 老师演示 ③ 写实验报告时直接截图
// =====================================================================
static int runDemo() {
#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif
  std::cout << "========================================\n";
  std::cout << "   [演示模式] 上海地铁核心功能自动化测试\n";
  std::cout << "========================================\n\n";

  // 1) 加载
  StationManager sm;
  if (!sm.loadFromCSV(STATION_CSV)) {
    std::cerr << "[错误] 加载 " << STATION_CSV << " 失败！\n";
    return 1;
  }
  std::cout << "[1] 加载站点: " << sm.allStations().size() << " 个\n";

  sm.loadInitFromCSV(STATION_INIT);

  Graph graph(sm);
  if (!graph.build(EDGE_CSV)) {
    std::cerr << "[错误] 构建图失败！\n";
    return 1;
  }
  std::cout << "[1] 构建图: " << graph.nodeCount() << " 个顶点\n\n";

  PathFinder pf(graph, sm);

  // ---- 工具：按 name 拿第一个匹配 id（同名换乘站优先 1 号线） ----
  auto pickId = [&](const std::string& name, const std::string& preferLine = "") {
    auto sts = sm.getStationsByName(name);
    if (sts.empty()) return -1;
    if (!preferLine.empty()) {
      for (auto& s : sts) if (s.line == preferLine) return s.id;
    }
    return sts[0].id;
  };

  // ---- 测试用例集合 ----
  struct Case { std::string title; std::string s; std::string sLine; std::string e; std::string eLine; };
  std::vector<Case> cases = {
    {"【T1】莘庄 -> 人民广场 (1号线 直达)",
     "莘庄", "1号线", "人民广场", "1号线"},
    {"【T2】莘庄 -> 陆家嘴 (1→2 在 人民广场)",
     "莘庄", "1号线", "陆家嘴", "2号线"},
    {"【T3】上海火车站 -> 浦东国际机场 (跨多线)",
     "上海火车站", "1号线", "浦东国际机场", "2号线"},
    {"【T4】徐家汇 -> 世纪公园 (1→2 在 人民广场)",
     "徐家汇", "1号线", "世纪公园", "2号线"},
  };

  for (size_t i = 0; i < cases.size(); ++i) {
    auto& c = cases[i];
    int sId = pickId(c.s, c.sLine);
    int eId = pickId(c.e, c.eLine);
    if (sId < 0 || eId < 0) {
      std::cout << c.title << "\n  [跳过] 找不到起终点\n\n";
      continue;
    }
    std::cout << "===== " << c.title << " =====\n";
    Path p = pf.findBestByTime(sId, eId);
    std::cout << p.toPrettyString(sm) << "\n\n";
  }

  // ---- K 短路演示 ----
  std::cout << "===== 【K 短路】莘庄 -> 陆家嘴 (前 3 条, 按时间排序) =====\n";
  int sId = pickId("莘庄", "1号线");
  int eId = pickId("陆家嘴", "2号线");
  auto paths = pf.findKShortestByTime(sId, eId, 3);
  for (size_t i = 0; i < paths.size(); ++i) {
    std::cout << "  ---- 候选 " << (i + 1) << " ----\n";
    std::cout << paths[i].toPrettyString(sm) << "\n";
  }
  std::cout << "\n";

  // ---- 边界测试：关闭 人民广场(1号线) 后自动绕行 ----
  std::cout << "===== 【边界】关闭 人民广场(1号线) 后, 莘庄 -> 陆家嘴 =====\n";
  sm.setStationStatus("人民广场", "1号线", "关闭");
  Path p2 = pf.findBestByTime(sId, eId);
  std::cout << p2.toPrettyString(sm) << "\n\n";
  sm.restoreInitialStatus();  // 恢复

  // ---- 受影响区域分析 ----
  std::cout << "===== 【受影响分析】如果关闭 世纪公园 (2号线) =====\n";
  auto info = pf.analyzeImpact("世纪公园", "2号线");
  std::cout << "  站点: " << info.name << " (" << info.line << ")\n";
  std::cout << "  影响等级: " << info.level << "\n";
  std::cout << "  受影响线路数: " << info.affectedLines.size() << "\n";
  std::cout << "  同线相邻数: " << info.sameLineAdj.size() << "\n";
  std::cout << "  关闭后变孤立的站点数: " << info.noAdj.size() << "\n\n";

  std::cout << "========================================\n";
  std::cout << "  演示完成。\n";
  std::cout << "========================================\n";
  return 0;
}

// =====================================================================
//   主入口
// =====================================================================
int main(int argc, char** argv) {
#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif

  // ---- 命令行参数解析 ----
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--demo" || a == "-d")   return runDemo();
    if (a == "--help" || a == "-h" || a == "/?") {
      std::cout
        << "用法:\n"
        << "  metro.exe              进入交互式菜单\n"
        << "  metro.exe --demo       跑预设测试用例 (一键演示)\n"
        << "  metro.exe --help       显示本帮助\n";
      return 0;
    }
  }

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

// =====================================================================
//   Unity Build
//   把所有 .cpp 直接 include 进来，使 `g++ main.cpp` 一次编译即可。
//   传统多文件编译请加 -DNO_UNITY_BUILD（见本文件顶部说明）。
// =====================================================================
#ifndef NO_UNITY_BUILD
  #include "station.cpp"
  #include "graph.cpp"
  #include "pathfinder.cpp"
  #include "menu.cpp"
#endif
