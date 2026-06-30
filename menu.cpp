// =============================================================
//  menu.cpp
//  控制台菜单实现（v1.3 - 对齐运行效果图格式）
// =============================================================
#include "menu.h"
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>


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
  // 去掉首尾 ASCII 空白
  size_t b = 0, e = s.size();
  auto isSpace = [](unsigned char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
  };
  while (b < e && isSpace((unsigned char)s[b])) ++b;
  while (e > b && isSpace((unsigned char)s[e - 1])) --e;
  s = s.substr(b, e - b);
  if (s.empty()) return defaultVal;
  // 严格校验: 只允许 [可选 +/-] + 全数字, 拒绝浮点数 / 字母 / 符号混杂
  size_t i = 0;
  if (s[0] == '+' || s[0] == '-') ++i;
  if (i == s.size()) return defaultVal;   // 仅符号无数字
  for (; i < s.size(); ++i) {
    if (!std::isdigit((unsigned char)s[i])) return defaultVal;
  }
  try { return std::stoi(s); } catch (...) { return defaultVal; }
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

// 把 "1" "1号线" "1号" 都归一为 "X号线"
static std::string normalizeLineName(const std::string &s) {
  if (s.empty()) return s;
  // 全角数字 → 半角（用字符串匹配避免多字节警告）
  static const std::string fullDigits = "０１２３４５６７８９";
  std::string t;
  for (size_t i = 0; i < s.size(); ) {
    bool matched = false;
    for (int d = 0; d <= 9; ++d) {
      std::string full(1, (char)(0xEF));
      full += (char)(0x82);
      full += (char)(0x80 + d);
      // 这里用直接字节比较更稳妥
      if (i + 3 <= s.size()
          && (unsigned char)s[i]     == (unsigned char)fullDigits[d*3]
          && (unsigned char)s[i + 1] == (unsigned char)fullDigits[d*3 + 1]
          && (unsigned char)s[i + 2] == (unsigned char)fullDigits[d*3 + 2]) {
        t.push_back('0' + d);
        i += 3;
        matched = true;
        break;
      }
    }
    if (!matched) { t.push_back(s[i]); ++i; }
  }
  // 已经是 X号线
  if (t.find("号线") != std::string::npos) return t;
  // 纯数字 → 加 "号线"
  bool allDigit = !t.empty();
  for (char c : t) if (!std::isdigit((unsigned char)c)) { allDigit = false; break; }
  if (allDigit) return t + "号线";
  return t;
}


// ============================================================
//    模糊搜索 + 换乘站二次选择（按运行效果图格式）
//    输出:
//      匹配站点如下：
//      1. 莘庄 (1号线)
//      2. 莘庄 (5号线)
//      请输入对应编号选择站点：
// ============================================================
int Menu::fuzzyPickStation(const std::string &prompt) {
  while (true) {
    std::string key = readLine(prompt);
    if (key.empty()) return -1;

    auto hits = sm_.fuzzySearch(key);
    if (hits.empty()) {
      std::cout << "  [提示] 未找到匹配站点，请重新输入。\n";
      continue;
    }

    if (hits.size() == 1) {
      std::cout << "  匹配: " << hits[0].name << " (" << hits[0].line << ")\n";
      return hits[0].id;
    }

    std::cout << "  匹配站点如下：\n";
    for (size_t i = 0; i < hits.size(); ++i) {
      std::cout << "  " << (i + 1) << ". " << hits[i].name
                << " (" << hits[i].line << ")\n";
    }
    int idx = readInt("  请输入对应编号选择站点：");
    if (idx <= 0 || (size_t)idx > hits.size()) {
      std::cout << "  [提示] 编号无效，请重新输入站点名。\n";
      continue;
    }
    return hits[idx - 1].id;
  }
}


// ============================================================
//                      主菜单
//   按指导书 §3.8 运行效果图：
//   1. 线路站点信息/运营状态管理
//   2. 所需时间最短路径规划
//   3. 所需换乘次数最少路径规划
//   4. 退出系统
// ============================================================
void Menu::run() {
  while (true) {
    clearScreen();
    std::cout << "========================================\n";
    std::cout << "  上海地铁路径规划与管理系统\n";
    std::cout << "  (Shanghai Metro Routing & Management)\n";
    std::cout << "========================================\n";
    std::cout << "  1. 线路站点信息/运营状态管理\n";
    std::cout << "  2. 所需时间最短路径规划\n";
    std::cout << "  3. 所需换乘次数最少路径规划\n";
    std::cout << "  4. 退出系统\n";
    std::cout << "========================================\n";
    int ch = readInt("  请输入选项编号：");
    switch (ch) {
      case 1: stationMenu();        break;
      case 2: timeMenu();           break;
      case 3: transferMenu();       break;
      case 4:
        std::cout << "  感谢使用，再见！\n";
        return;
      default:
        std::cout << "  [提示] 输入无效，请输入数字选项1、2、3、4。\n"; pause();
    }
  }
}


// ============================================================
//   站点管理子菜单（按运行效果图 §3.8.1 + 运营管理扩展）
//   1.批量更新 2.手工更新 3.显示关闭站点 4.恢复初始
//   5.显示线路站点 6.换乘站整体关闭 7.线路停运管理
//   8.全网停运管理 9.受影响区域分析 10.网络连通性分析
//   11.保存数据 12.返回上级菜单
// ============================================================
void Menu::stationMenu() {
  while (true) {
    clearScreen();
    std::cout << "--- 线路站点信息/运营状态管理 ---\n";
    std::cout << "1. 从 CSV 文件批量更新站点开启/关闭状态\n";
    std::cout << "2. 手工更新站点开启/关闭状态\n";
    std::cout << "3. 显示当前关闭站点\n";
    std::cout << "4. 恢复所有站点初始状态\n";
    std::cout << "5. 显示线路站点信息\n";
    std::cout << "6. 换乘站整体关闭管理\n";
    std::cout << "7. 线路停运管理\n";
    std::cout << "8. 全网停运/恢复管理\n";
    std::cout << "9. 受关闭站点影响站点分析\n";
    std::cout << "10. 网络连通性分析\n";
    std::cout << "11. 保存当前数据\n";
    std::cout << "12. 返回上级菜单\n";
    int ch = readInt("请输入选项编号：");
    switch (ch) {
      case 1:  batchUpdateFromCSV();       pause(); break;
      case 2:  toggleOneStation();         pause(); break;
      case 3:  showClosedStations();       pause(); break;
      case 4:  restoreInitial();           pause(); break;
      case 5:  showLineStations();         pause(); break;
      case 6:  closeTransferStationMenu(); pause(); break;
      case 7:  lineOutageMenu();           pause(); break;
      case 8:  networkOutageMenu();        pause(); break;
      case 9:  runImpactAnalysis();        pause(); break;
      case 10: networkConnectivityMenu();  pause(); break;
      case 11: saveData();                 pause(); break;
      case 12: return;
      default: std::cout << "  [提示] 无效选项。\n"; pause();
    }
  }
}


// ---------- 1) 批量更新 ----------
void Menu::batchUpdateFromCSV() {
  clearScreen();
  std::cout << "--- 批量更新站点状态 ---\n";
  std::cout << "  [提示] 支持两种格式：id,status 或 name,line,status\n";
  std::string input = readLine(
      "  请输入 CSV 路径（直接回车使用 data/update_station_status.csv，也可直接输入一条记录）: ");
  if (input.empty()) input = "data/update_station_status.csv";

  auto fields = StationManager::parseCSVLine(input);
  bool handled = false;
  bool ok = false;
  if (fields.size() == 2) {
    try {
      int id = std::stoi(StationManager::trim(fields[0]));
      const Station *s = sm_.findById(id);
      std::string status = StationManager::trim(fields[1]);
      if (s && (status == "开启" || status == "关闭")) {
        handled = true;
        ok = sm_.setStationStatus(s->name, s->line, status);
      }
    } catch (...) {
    }
  } else if (fields.size() >= 3) {
    std::string name = StationManager::trim(fields[0]);
    std::string line = StationManager::trim(fields[1]);
    std::string status = StationManager::trim(fields[2]);
    if (!name.empty() && !line.empty() &&
        (status == "开启" || status == "关闭")) {
      handled = true;
      ok = sm_.setStationStatus(name, line, status);
    }
  }

  if (handled) {
    if (ok)
      std::cout << "  [完成] 单条站点状态更新成功。\n";
    else
      std::cout << "  [失败] 单条站点状态更新失败，请检查站点名、线路名或状态值。\n";
    return;
  }

  if (sm_.batchUpdateFromCSV(input) >= 0)
    std::cout << "  [完成] 批量更新已执行。\n";
  else
    std::cout << "  [失败] 更新文件不存在或无法打开。\n";
}


// ---------- 2) 手工更新站点状态（支持循环操作 + 统计）----------
void Menu::toggleOneStation() {
  clearScreen();
  std::cout << "--- 手工更新站点状态 ---\n";
  int modifiedCnt = 0;
  while (true) {
    int id = fuzzyPickStation("  请输入待修改站点关键词（exit退出）：");
    if (id < 0) break;
    const Station *s = sm_.findById(id);
    if (!s) continue;
    std::cout << "  " << s->name << "," << s->line << "," << s->status << "\n";
    std::string newSt = readLine("  请输入站点状态（开启/关闭）> ");
    if (newSt != "开启" && newSt != "关闭") {
      std::cout << "  [提示] 状态只能是 开启/关闭。\n";
      continue;
    }
    if (sm_.setStationStatus(s->name, s->line, newSt)) {
      std::cout << "  修改站点: " << s->name << " (" << s->line
                << ") -> 状态: " << newSt << "\n";
      ++modifiedCnt;
    } else {
      std::cout << "  [失败] 状态切换失败。\n";
    }
  }
  if (modifiedCnt > 0)
    std::cout << modifiedCnt << " 个站点的状态修改完成。\n";
  else
    std::cout << "  没有修改任何站点。\n";
}


// ---------- 3) 显示当前关闭站点 ----------
void Menu::showClosedStations() {
  clearScreen();
  std::cout << "--- 当前关闭的站点 ---\n";
  auto closed = sm_.getClosedStations();
  if (closed.empty()) {
    std::cout << "  所有站点均处于开放状态。\n";
    return;
  }
  for (size_t i = 0; i < closed.size(); ++i) {
    std::cout << "  " << (i + 1) << ". " << closed[i].name
              << " (" << closed[i].line << ") - " << closed[i].status << "\n";
  }
  std::cout << "  共 " << closed.size() << " 个关闭站点。\n";
}


// ---------- 4) 恢复所有站点初始状态 (Y/N 确认) ----------
void Menu::restoreInitial() {
  clearScreen();
  std::cout << "--- 恢复所有站点初始状态 ---\n";
  std::string answer = readLine("  您确定要恢复所有站点的初始状态?（Y/N）");
  if (answer == "Y" || answer == "y" || answer == "是") {
    sm_.restoreInitialStatus();
    std::cout << "  已成功恢复至初始状态。\n";
  } else {
    std::cout << "  已取消恢复操作。\n";
  }
}


// ---------- 5) 显示线路站点信息（按运行效果图 §3.8.1）----------
void Menu::showLineStations() {
  clearScreen();
  std::cout << "--- 显示线路站点信息 ---\n";
  auto allLines = sm_.getAllLines();
  if (allLines.empty()) {
    std::cout << "  [提示] 当前没有可查询的线路数据。\n";
    return;
  }

  std::cout << "  当前可查询的地铁线路有：\n";
  for (size_t i = 0; i < allLines.size(); ++i) {
    std::cout << "  " << (i + 1) << ". " << allLines[i] << "\n";
  }

  int choice = readInt("  请输入要查询的线路序号：", -1);
  if (choice < 0)
    return;
  if (choice <= 0 || (size_t)choice > allLines.size()) {
    std::cout << "  [提示] 线路编号无效。\n";
    return;
  }

  std::string line = allLines[choice - 1];
  auto sts = sm_.getStationsByLine(line);
  std::cout << "  " << line << " 共有 " << sts.size() << " 个站点：\n";

  // 对每个站点：判断是否为换乘站（同名存在多条线路 → 是换乘站）
  for (size_t i = 0; i < sts.size(); ++i) {
    const auto &s = sts[i];
    auto same = sm_.getStationsByName(s.name);
    bool isTransfer = (same.size() > 1);
    std::cout << "  ○ " << s.name;
    if (isTransfer) {
      std::cout << " (换乘 ";
      for (size_t k = 0; k < same.size(); ++k) {
        if (same[k].line == s.line) continue;
        std::cout << same[k].line;
        break;        // 仅显示一个换乘线路，简洁
      }
      std::cout << ")";
    }
    std::cout << "\n";
  }
}


// ---------- 7) 站点查询 ----------
void Menu::queryStations() {
  clearScreen();
  std::cout << "--- 站点查询 ---\n";
  std::string key = readLine("请输入站点名称：");
  key = StationManager::trim(key);
  if (key.empty()) {
    std::cout << "  [提示] 站点名称不能为空。\n";
    return;
  }

  auto hits = sm_.fuzzySearch(key);
  if (hits.empty()) {
    std::cout << "  [提示] 未找到匹配站点。\n";
    return;
  }

  std::cout << "匹配的站点如下:\n";
  for (size_t i = 0; i < hits.size(); ++i) {
    std::cout << (i + 1) << ". " << hits[i].name
              << "（" << hits[i].line << "）\n";
  }
}


// ============================================================
//   时间最优子菜单（独立二级菜单，按 §3.8 运行效果图）
//   1. 单条所需时间最短路径
//   2. 3条所需时间最短路径
//   3. 返回上级菜单
// ============================================================
void Menu::timeMenu() {
  while (true) {
    clearScreen();
    std::cout << "--- 所需时间最短路径规划 ---\n";
    std::cout << "1. 单条所需时间最短路径\n";
    std::cout << "2. 3条所需时间最短路径\n";
    std::cout << "3. 返回上级菜单\n";
    int ch = readInt("请输入选项编号：");
    switch (ch) {
      case 1: runPathQuery(0); pause(); break;
      case 2: runPathQuery(2); pause(); break;
      case 3: return;
      default: std::cout << "  [提示] 无效选项。\n"; pause();
    }
  }
}

// ============================================================
//   换乘最优子菜单（独立二级菜单，按 §3.8 运行效果图）
//   1. 单条换乘次数最少路径
//   2. 3条换乘次数最少路径
//   3. 返回主菜单
// ============================================================
void Menu::transferMenu() {
  while (true) {
    clearScreen();
    std::cout << "--- 所需换乘次数最少路径规划 ---\n";
    std::cout << "1. 单条换乘次数最少路径\n";
    std::cout << "2. 3条换乘次数最少路径\n";
    std::cout << "3. 返回主菜单\n";
    int ch = readInt("请输入选项编号：");
    switch (ch) {
      case 1: runPathQuery(1); pause(); break;
      case 2: runPathQuery(3); pause(); break;
      case 3: return;
      default: std::cout << "  [提示] 无效选项。\n"; pause();
    }
  }
}


// ---------- 路径查询核心 ----------
void Menu::runPathQuery(int mode) {
  clearScreen();
  const char *titles[] = {"单条所需时间最短路径",
                          "单条换乘次数最少路径",
                          "3 条所需时间最短路径",
                          "3 条换乘次数最少路径"};
  std::cout << "--- " << titles[mode] << " ---\n";

  int sId = fuzzyPickStation("请输入起点站关键词：");
  if (sId < 0) return;
  int eId = fuzzyPickStation("请输入终点站关键词：");
  if (eId < 0) return;
  if (sId == eId) {
    std::cout << "  [提示] 起点和终点相同，无需进行路径规划。\n";
    return;
  }
  const Station *ss = sm_.findById(sId);
  const Station *es = sm_.findById(eId);
  if (!ss || !es) return;

  // 前置检查起点/终点是否关闭
  if (ss->status == "关闭") {
    std::cout << "  起点：" << ss->name << "（" << ss->line
              << "）已关闭，无法进行路径规划。\n";
    return;
  }
  if (es->status == "关闭") {
    std::cout << "  终点：" << es->name << "（" << es->line
              << "）已关闭，无法进行路径规划。\n";
    return;
  }
  std::cout << "  起点：" << ss->name << " (" << ss->line << ")\n";
  std::cout << "  终点：" << es->name << " (" << es->line << ")\n\n";

  if (mode == 0) {
    // 单条时间最短
    Path p = pf_.findBestByTime(sId, eId);
    if (!p.valid) { std::cout << "  [提示] 未找到可达路径。\n"; return; }
    std::cout << p.toPrettyString(sm_) << "\n";
  } else if (mode == 1) {
    // 单条换乘最少
    Path p = pf_.findBestByTransfer(sId, eId);
    if (!p.valid) { std::cout << "  [提示] 未找到可达路径。\n"; return; }
    std::cout << p.toPrettyString(sm_) << "\n";
  } else if (mode == 2) {
    // K 条时间最短
    auto paths = pf_.findKShortestByTime(sId, eId, 3);
    if (paths.empty()) { std::cout << "  [提示] 未找到可达路径。\n"; return; }
    for (size_t i = 0; i < paths.size(); ++i) {
      std::cout << "  [候选 " << (i + 1) << "] "
                << paths[i].toPrettyString(sm_) << "\n";
    }
  } else if (mode == 3) {
    // K 条换乘最少
    auto paths = pf_.findKShortestByTransfer(sId, eId, 3);
    if (paths.empty()) { std::cout << "  [提示] 未找到可达路径。\n"; return; }
    for (size_t i = 0; i < paths.size(); ++i) {
      std::cout << "  [候选 " << (i + 1) << "] "
                << paths[i].toPrettyString(sm_) << "\n";
    }
  }
}


// ============================================================
//   受关闭站点影响站点分析（按运行效果图 §3.8.1 / §3.8.3）
// ============================================================
void Menu::runImpactAnalysis() {
  clearScreen();
  std::cout << "--- 受关闭站点影响站点分析 ---\n";
  int id = fuzzyPickStation("  请输入待分析站点关键字：");
  if (id < 0) return;
  const Station *s = sm_.findById(id);
  if (!s) return;

  auto info = pf_.analyzeImpact(s->name, s->line);

  std::cout << "==============================\n";
  std::cout << "关闭站点：" << info.name << "(" << info.line << ")\n";

  std::cout << "受影响线路：\n";
  if (info.affectedLines.empty()) {
    std::cout << "  (无)\n";
  } else {
    for (auto &ln : info.affectedLines) std::cout << "  " << ln << "\n";
  }

  std::cout << "\n直接影响相邻站点：\n";
  if (info.sameLineAdj.empty()) {
    std::cout << "  (无)\n";
  } else {
    std::unordered_set<int> isolated(info.noAdj.begin(), info.noAdj.end());
    for (int nid : info.sameLineAdj) {
      const Station *ns = sm_.findById(nid);
      std::cout << "  " << (ns ? ns->name : "?")
                << "(" << (ns ? ns->line : "?") << ")";
      if (isolated.count(nid))
        std::cout << " [关闭后将成为无相邻站点]";
      std::cout << "\n";
    }
  }

  std::cout << "\n影响等级：" << info.level << "\n";
}


// ============================================================
//      §3.3 建站管理（可选加分项）+ 运营管理扩展
// ============================================================
void Menu::buildMenu() {
  stationMenu();
}

// ---- 运营管理扩展：换乘站整体关闭 ----
void Menu::closeTransferStationMenu() {
  clearScreen();
  std::cout << "--- 换乘站整体关闭管理 ---\n";
  std::cout << "  [说明] 输入换乘站名称（如\"人民广场\"），将同时关闭该站所有线路的站点。\n";
  std::string name = readLine("  请输入换乘站名称：");
  if (name.empty()) return;
  name = StationManager::trim(name);
  int cnt = sm_.closeTransferStation(name);
  if (cnt == 0)
    std::cout << "  [提示] 操作未生效，请检查站点名称是否正确或是否为换乘站。\n";
  else
    std::cout << "  [完成] 已关闭 " << cnt << " 个站点。\n";
}

// ---- 运营管理扩展：线路停运管理 ----
void Menu::lineOutageMenu() {
  while (true) {
    clearScreen();
    std::cout << "--- 线路停运管理 ---\n";
    std::cout << "1. 停运指定线路\n";
    std::cout << "2. 恢复指定线路\n";
    std::cout << "3. 返回上级菜单\n";
    int ch = readInt("请输入选项编号：");
    if (ch == 3) return;
    if (ch != 1 && ch != 2) {
      std::cout << "  [提示] 无效选项。\n"; pause(); continue;
    }

    auto lines = sm_.getAllLines();
    std::cout << "  可选线路：\n";
    for (size_t i = 0; i < lines.size(); ++i)
      std::cout << "  " << (i + 1) << ". " << lines[i] << "\n";

    int lc = readInt("  请输入线路序号：", -1);
    if (lc <= 0 || (size_t)lc > lines.size()) {
      std::cout << "  [提示] 线路编号无效。\n"; pause(); continue;
    }
    std::string line = lines[lc - 1];

    if (ch == 1)
      sm_.closeLineStations(line);
    else
      sm_.openLineStations(line);
    pause();
  }
}

// ---- 运营管理扩展：全网停运/恢复 ----
void Menu::networkOutageMenu() {
  while (true) {
    clearScreen();
    std::cout << "--- 全网停运/恢复管理 ---\n";
    std::cout << "1. 全网停运（关闭所有站点）\n";
    std::cout << "2. 全网恢复（开启所有站点）\n";
    std::cout << "3. 返回上级菜单\n";
    int ch = readInt("请输入选项编号：");
    switch (ch) {
      case 1: {
        std::string ans = readLine("  确认全网停运?（Y/N）");
        if (ans == "Y" || ans == "y" || ans == "是")
          sm_.closeAllStations();
        else
          std::cout << "  已取消。\n";
        pause(); break;
      }
      case 2: {
        std::string ans = readLine("  确认全网恢复?（Y/N）");
        if (ans == "Y" || ans == "y" || ans == "是")
          sm_.openAllStations();
        else
          std::cout << "  已取消。\n";
        pause(); break;
      }
      case 3: return;
      default: std::cout << "  [提示] 无效选项。\n"; pause();
    }
  }
}

// ---- 运营管理扩展：网络连通性分析 ----
void Menu::networkConnectivityMenu() {
  clearScreen();
  std::cout << "--- 网络连通性分析 ---\n";
  auto ninfo = pf_.analyzeNetworkConnectivity();

  std::cout << "  开放站点数: " << ninfo.totalOpenStations << "\n";
  std::cout << "  关闭站点数: " << ninfo.totalClosedStations << "\n";
  std::cout << "  连通分量数: " << ninfo.componentCount << "\n";

  if (ninfo.isConnected) {
    std::cout << "  结论: 全网连通，所有开放站点之间可相互到达。\n";
  } else {
    std::cout << "  结论: 网络存在断裂！以下为各连通分量详情：\n";
    for (int i = 0; i < ninfo.componentCount; ++i) {
      const auto &comp = ninfo.components[i];
      std::cout << "  ---- 分量 " << (i + 1) << " (" << comp.size()
                << " 个站点) ----\n";
      if (comp.size() <= 20) {
        for (int id : comp) {
          const Station *s = sm_.findById(id);
          if (s) std::cout << "    " << s->name << "(" << s->line << ")\n";
        }
      } else {
        for (size_t j = 0; j < 5 && j < comp.size(); ++j) {
          const Station *s = sm_.findById(comp[j]);
          if (s) std::cout << "    " << s->name << "(" << s->line << ")\n";
        }
        std::cout << "    ... 共 " << comp.size() << " 个站点\n";
      }
    }
  }
}

// ---- 保存数据 (保留在主菜单外，通过站点管理子菜单访问) ----
void Menu::saveData() {
  clearScreen();
  std::cout << "--- 保存当前数据 ---\n";
  std::string path = readLine("  请输入保存路径（直接回车覆盖 data/Station.csv）: ");
  if (path.empty()) path = "data/Station.csv";
  if (sm_.saveCurrentToCSV(path))
    std::cout << "  [成功] 已保存到 " << path << "\n";
  else
    std::cout << "  [失败] 保存失败。\n";
}
