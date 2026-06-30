// =============================================================
//  menu.cpp
//  控制台菜单实现
// =============================================================
#include "menu.h"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>


// ============================================================
//                  通用 UI 工具
// ============================================================
void Menu::clearScreen() {
#ifdef _WIN32
  system("cls");
#else
  system("clear");
#endif
}
void Menu::pause() {
  std::cout << "\n>>> 按回车键继续...";
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  std::cin.get();
}
int Menu::readInt(const std::string &prompt, int defaultVal) {
  std::cout << prompt;
  std::string s;
  std::getline(std::cin, s);
  if (s.empty())
    return defaultVal;
  try {
    return std::stoi(s);
  } catch (...) {
    return defaultVal;
  }
}
std::string Menu::readLine(const std::string &prompt) {
  std::cout << prompt;
  std::string s;
  std::getline(std::cin, s);
  return s;
}
void Menu::printHeader(const std::string &title) {
  std::cout << "\n================ " << title << " ================\n";
}
void Menu::printStations(const std::vector<Station> &sts) {
  if (sts.empty()) {
    std::cout << "  (无结果)\n";
    return;
  }
  std::cout << "  " << std::left << std::setw(8) << "ID" << std::setw(20)
            << "站名" << std::setw(10) << "线路" << std::setw(8) << "状态"
            << "\n";
  std::cout << "  " << std::string(46, '-') << "\n";
  for (const auto &s : sts) {
    std::cout << "  " << std::left << std::setw(8) << s.id << std::setw(20)
              << s.name << std::setw(10) << s.line << std::setw(8) << s.status
              << "\n";
  }
}

// ============================================================
//            模糊搜索 + 换乘站二次选择
// ============================================================
//
//  1) 用户输入关键字（支持部分汉字）
//  2) 调用 sm_.fuzzySearch(keyword) 拿到所有命中
//  3) 如果命中 0 条 → 返回 -1
//  4) 如果命中 1 条 → 直接返回 id
//  5) 如果多条 → 列出，用户输入编号选择；输入 0 取消
// ============================================================
int Menu::fuzzyPickStation(const std::string &prompt) {
  while (true) {
    std::string key = readLine(prompt);
    if (key.empty())
      return -1;
    auto hits = sm_.fuzzySearch(key);
    if (hits.empty()) {
      std::cout << "  [提示] 未找到匹配站点，请重新输入（直接回车可取消）。\n";
      continue;
    }
    if (hits.size() == 1)
      return hits[0].id;

    // 多条命中：先按 name 分组，提示换乘
    std::cout << "\n  命中 " << hits.size() << " 条结果：\n";
    printStations(hits);
    std::cout << "  请输入对应 ID（0 取消）：";
    int id = readInt("");
    if (id == 0)
      return -1;
    // 校验
    for (const auto &s : hits)
      if (s.id == id)
        return id;
    std::cout << "  [提示] 输入的 ID 不在候选列表中。\n";
  }
}

// ============================================================
//                      主菜单
// ============================================================
void Menu::run() {
    while (true) {
        clearScreen();
        std::cout << "========================================\n";
        std::cout << "  上海地铁路径规划与管理系统\n";
        std::cout << "  (Shanghai Metro Routing & Management)\n";
        std::cout << "========================================\n";
        std::cout << "  1. 站点管理\n";
        std::cout << "  2. 路径查询\n";
        std::cout << "  3. 受影响区域分析\n";
        std::cout << "  4. 建站管理（添加 / 删除站点 / 加边）\n";
        std::cout << "  5. 保存当前状态到 Station.csv\n";
        std::cout << "  0. 退出\n";
        std::cout << "========================================\n";
        int ch = readInt("  请选择: ");
        switch (ch) {
            case 1: stationMenu();  break;
            case 2: pathMenu();     break;
            case 3: impactMenu();   break;
            case 4: buildMenu();    break;
            case 5: saveData();     pause(); break;
            case 0:
                std::cout << "  再见！\n";
                return;
            default:
                std::cout << "  [提示] 无效选择。\n";
                pause();
        }
    }
}

// ============================================================
//                    站点管理子菜单
// ============================================================
void Menu::stationMenu() {
  while (true) {
    clearScreen();
    printHeader("站点管理");
    std::cout << "  1. 显示所有关闭站点\n";
    std::cout << "  2. 显示指定线路的所有站点\n";
    std::cout << "  3. 切换单个站点状态（开启/关闭）\n";
    std::cout << "  4. 批量更新（从 update_station_status.csv）\n";
    std::cout << "  5. 恢复初始状态（Station_init.csv）\n";
    std::cout << "  0. 返回上一级\n";
    int ch = readInt("  请选择: ");
    switch (ch) {
    case 1:
      showClosedStations();
      pause();
      break;
    case 2:
      showLineStations();
      pause();
      break;
    case 3:
      toggleOneStation();
      pause();
      break;
    case 4:
      batchUpdateFromCSV();
      pause();
      break;
    case 5:
      restoreInitial();
      pause();
      break;
    case 0:
      return;
    default:
      std::cout << "  [提示] 无效选择。\n";
      pause();
    }
  }
}

// ---------- 1) 关闭站点名单 ----------
void Menu::showClosedStations() {
  printHeader("当前关闭的站点");
  auto closed = sm_.getClosedStations();
  if (closed.empty()) {
    std::cout << "  （当前没有关闭的站点）\n";
    return;
  }
  printStations(closed);
  std::cout << "  共 " << closed.size() << " 个关闭站点。\n";
}

// ---------- 2) 指定线路的所有站点 ----------
void Menu::showLineStations() {
  printHeader("按线路查看站点");
  std::string line = readLine("  请输入线路名（如 1号线 / 2号线）: ");
  if (line.empty())
    return;
  auto sts = sm_.getStationsByLine(line);
  if (sts.empty()) {
    std::cout << "  [提示] 没有该线路的站点，请检查线路名。\n";
    return;
  }
  std::cout << "  线路 [" << line << "] 共有 " << sts.size() << " 个站点：\n";
  printStations(sts);
}

// ---------- 3) 切换单个站点 ----------
void Menu::toggleOneStation() {
  printHeader("切换站点状态");
  int id = fuzzyPickStation("  请输入要切换的站点名（部分汉字可）: ");
  if (id < 0)
    return;
  const Station *s = sm_.findById(id);
  if (!s)
    return;
  std::cout << "  选中: " << s->name << " (" << s->line
            << "), 当前状态: " << s->status << "\n";
  std::string newSt = (s->status == "开启") ? "关闭" : "开启";
  int confirm = readInt("  确认切换为 [" + newSt + "]? 1=是 0=否 : ");
  if (confirm == 1) {
    if (sm_.setStationStatus(s->name, s->line, newSt)) {
      std::cout << "  [成功] " << s->name << " (" << s->line << ") 已切换为 "
                << newSt << "。\n";
    } else {
      std::cout << "  [失败] 状态切换失败。\n";
    }
  }
}

// ---------- 4) 批量更新 ----------
void Menu::batchUpdateFromCSV() {
  printHeader("批量更新站点状态");
  std::string path =
      readLine("  请输入 CSV 路径（直接回车用 update_station_status.csv）: ");
  if (path.empty())
    path = "update_station_status.csv";
  sm_.batchUpdateFromCSV(path);
}

// ---------- 5) 恢复初始状态 ----------
void Menu::restoreInitial() {
  printHeader("恢复初始状态");
  int confirm = readInt("  确认恢复? 1=是 0=否 : ");
  if (confirm == 1)
    sm_.restoreInitialStatus();
}

// ============================================================
//                  路径查询子菜单
// ============================================================
void Menu::pathMenu() {
  while (true) {
    clearScreen();
    printHeader("路径查询");
    std::cout << "  1. 单条最优：时间最短（换乘最少次之）\n";
    std::cout << "  2. 单条最优：换乘最少（时间最短次之）\n";
    std::cout << "  3. 时间最短的前 3 条路径（K 短路）\n";
    std::cout << "  4. 换乘最少的前 3 条路径（K 短路）\n";
    std::cout << "  0. 返回上一级\n";
    int ch = readInt("  请选择: ");
    switch (ch) {
    case 1:
      runPathQuery(0);
      pause();
      break;
    case 2:
      runPathQuery(1);
      pause();
      break;
    case 3:
      runPathQuery(2);
      pause();
      break;
    case 4:
      runPathQuery(3);
      pause();
      break;
    case 0:
      return;
    default:
      std::cout << "  [提示] 无效选择。\n";
      pause();
    }
  }
}

// ---------- 路径查询核心 ----------
void Menu::runPathQuery(int mode) {
  clearScreen();
  const char *titles[] = {"时间最短的路径", "换乘最少的路径",
                          "时间最短的前 3 条路径", "换乘最少的前 3 条路径"};
  printHeader(titles[mode]);

  int sId = fuzzyPickStation("  请输入起点站名（部分汉字可）: ");
  if (sId < 0)
    return;
  int eId = fuzzyPickStation("  请输入终点站名（部分汉字可）: ");
  if (eId < 0)
    return;
  if (sId == eId) {
    std::cout << "  [提示] 起点与终点相同。\n";
    return;
  }
  const Station *ss = sm_.findById(sId);
  const Station *es = sm_.findById(eId);
  std::cout << "  起点: " << ss->name << " (" << ss->line << ")\n";
  std::cout << "  终点: " << es->name << " (" << es->line << ")\n\n";

  if (mode == 0) {
    Path p = pf_.findBestByTime(sId, eId);
    if (!p.valid) {
      std::cout << "  [提示] 未找到可达路径。\n";
      return;
    }
    std::cout << p.toPrettyString(sm_) << "\n";
  } else if (mode == 1) {
    Path p = pf_.findBestByTransfer(sId, eId);
    if (!p.valid) {
      std::cout << "  [提示] 未找到可达路径。\n";
      return;
    }
    std::cout << p.toPrettyString(sm_) << "\n";
  } else if (mode == 2) {
    auto paths = pf_.findKShortestByTime(sId, eId, 3);
    if (paths.empty()) {
      std::cout << "  [提示] 未找到可达路径。\n";
      return;
    }
    for (size_t i = 0; i < paths.size(); ++i) {
      std::cout << "  ------- 候选 " << (i + 1) << " -------\n";
      std::cout << paths[i].toPrettyString(sm_) << "\n";
    }
  } else if (mode == 3) {
    auto paths = pf_.findKShortestByTransfer(sId, eId, 3);
    if (paths.empty()) {
      std::cout << "  [提示] 未找到可达路径。\n";
      return;
    }
    for (size_t i = 0; i < paths.size(); ++i) {
      std::cout << "  ------- 候选 " << (i + 1) << " -------\n";
      std::cout << paths[i].toPrettyString(sm_) << "\n";
    }
  }
}

// ============================================================
//                 受影响区域分析
// ============================================================
void Menu::impactMenu() {
  clearScreen();
  printHeader("受影响区域分析");
  int id = fuzzyPickStation("  请输入要分析的站点名（部分汉字可）: ");
  if (id < 0)
    return;
  const Station *s = sm_.findById(id);
  if (!s)
    return;
  auto info = pf_.analyzeImpact(s->name, s->line);
  std::cout << "  站点: " << info.name << " (" << info.line << ")\n";
  std::cout << "  影响等级: " << info.level << "\n";
  std::cout << "  受影响线路: ";
  for (auto &ln : info.affectedLines)
    std::cout << ln << " ";
  std::cout << "\n";

  std::cout << "  同线相邻站点（" << info.sameLineAdj.size() << " 个）:\n";
  if (info.sameLineAdj.empty())
    std::cout << "    (无)\n";
  for (int nid : info.sameLineAdj) {
    const Station *ns = sm_.findById(nid);
    std::cout << "    - " << (ns ? ns->name : "?") << "\n";
  }
  std::cout << "  关闭后变成无相邻的站点（" << info.noAdj.size() << " 个）:\n";
  if (info.noAdj.empty())
    std::cout << "    (无)\n";
  for (int nid : info.noAdj) {
    const Station *ns = sm_.findById(nid);
    std::cout << "    - " << (ns ? ns->name : "?") << "\n";
  }
  pause();
}

// ============================================================
//                     保存数据
// ============================================================
void Menu::saveData() {
  printHeader("保存当前状态");
  std::string path = readLine("  请输入保存路径（直接回车覆盖 Station.csv）: ");
  if (path.empty())
    path = "data/Station.csv";
  if (sm_.saveCurrentToCSV(path))
    std::cout << "  [成功] 已保存到 " << path << "\n";
  else
    std::cout << "  [失败] 保存失败。\n";
}

// ============================================================
//      §3.3 建站管理（可选加分项）
// ============================================================
void Menu::buildMenu() {
  while (true) {
    clearScreen();
    printHeader("建站管理");
    std::cout << "  1. 添加新站点\n";
    std::cout << "  2. 删除站点\n";
    std::cout << "  3. 手动添加一条边\n";
    std::cout << "  0. 返回上一级\n";
    int ch = readInt("  请选择: ");
    switch (ch) {
      case 1: addNewStation();          pause(); break;
      case 2: removeExistingStation();   pause(); break;
      case 3: addNewEdgeInteractive();  pause(); break;
      case 0: return;
      default: std::cout << "  [提示] 无效选择。\n"; pause();
    }
  }
}

// ---- 添加新站点 ----
void Menu::addNewStation() {
  printHeader("添加新站点");
  std::string name = readLine("  请输入新站点名: ");
  if (name.empty()) { std::cout << "  [取消] 名称不能为空。\n"; return; }
  std::string line = readLine("  请输入所属线路（如 1号线）: ");
  if (line.empty()) { std::cout << "  [取消] 线路不能为空。\n"; return; }
  std::string st   = readLine("  状态 [开启/关闭]，默认开启: ");
  if (st.empty()) st = "开启";
  if (st != "开启" && st != "关闭") {
    std::cout << "  [错误] 状态只能是 开启/关闭。\n"; return;
  }
  int newId = sm_.addStation(name, line, st);
  if (newId < 0) {
    std::cout << "  [失败] 已存在同名+同线路的站点。\n";
    return;
  }
  std::cout << "  [成功] 已添加站点 ID=" << newId
            << " (" << name << " / " << line << " / " << st << ")\n";
  std::cout << "  [提示] 若需把新站点接入网络，请到『手动添加一条边』。\n";
}

// ---- 删除站点 ----
void Menu::removeExistingStation() {
  printHeader("删除站点");
  int id = fuzzyPickStation("  请输入要删除的站点名（部分汉字可）: ");
  if (id < 0) return;
  const Station *s = sm_.findById(id);
  if (!s) return;
  std::cout << "  选中: " << s->name << " (" << s->line << ")\n";
  int confirm = readInt("  确认删除? 1=是 0=否 : ");
  if (confirm == 1) {
    if (sm_.removeStation(s->name, s->line)) {
      std::cout << "  [成功] 站点已删除（需重新加载图才能反映在路径中）。\n";
      std::cout << "  [提示] 建议执行『手动添加一条边』或重启程序。\n";
    } else {
      std::cout << "  [失败] 删除失败。\n";
    }
  }
}

// ---- 手动添加边 ----
void Menu::addNewEdgeInteractive() {
  printHeader("手动添加一条边");
  int u = fuzzyPickStation("  请输入起点站: ");
  if (u < 0) return;
  int v = fuzzyPickStation("  请输入终点站: ");
  if (v < 0) return;
  std::string line = readLine("  请输入所属线路（如 1号线）: ");
  if (line.empty()) { std::cout << "  [取消] 线路不能为空。\n"; return; }
  int t = readInt("  请输入通行时间（分钟，默认 3）: ", 3);
  if (graph_.addEdge(u, v, line, t)) {
    std::cout << "  [成功] 已添加 " << u << " -> " << v
              << " (" << line << ", " << t << " min)\n";
  } else {
    std::cout << "  [失败] 起点或终点不存在。\n";
  }
}
