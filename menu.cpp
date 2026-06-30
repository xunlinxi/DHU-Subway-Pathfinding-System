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
    key = StationManager::trim(key);
    if (key == "exit")
      return -1;
    if (key.empty()) return -1;

    auto hits = stationManager_.fuzzySearch(key);
    if (hits.empty()) {
      std::cout << "未找到匹配的站点,请重新选择。\n";
      continue;
    }

    if (hits.size() == 1) {
      std::cout << "匹配: " << hits[0].name << "（" << hits[0].line << "）\n";
      return hits[0].id;
    }

    std::cout << "匹配的站点如下：\n";
    for (size_t i = 0; i < hits.size(); ++i) {
      std::cout << (i + 1) << ". " << hits[i].name
                << "（" << hits[i].line << "）\n";
    }
    int idx = readInt("请输入对应编号选择站点：");
    if (idx <= 0 || (size_t)idx > hits.size()) {
      std::cout << "编号无效，请重新输入站点名。\n";
      continue;
    }
    return hits[idx - 1].id;
  }
}


// ============================================================
//                      主菜单
// ============================================================
void Menu::run() {
  while (true) {
    clearScreen();
    std::cout << "==== 地铁路径规划系统 ====\n";
    std::cout << "1. 线路站点信息/运营状态管理\n";
    std::cout << "2. 所需时间最短路径规划\n";
    std::cout << "3. 所需换乘次数最少路径规划\n";
    std::cout << "4. 退出系统\n";
    int ch = readInt("请输入选项编号:");
    switch (ch) {
      case 1: stationMenu(); break;
      case 2: timePathMenu(); break;
      case 3: transferPathMenu(); break;
      case 4:
        std::cout << "退出系统\n";
        return;
      default:
        std::cout << "输入无效，请输入数字选项1、2、3、4\n";
        pause();
    }
  }
}


// ============================================================
//   站点管理子菜单（按运行效果图 §3.8.1）
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
    std::cout << "6. 返回上级菜单\n";
    int ch = readInt("请输入选项编号:");
    switch (ch) {
      case 1: batchUpdateFromCSV();   pause(); break;
      case 2: toggleOneStation();     pause(); break;
      case 3: showClosedStations();   pause(); break;
      case 4: restoreInitial();       pause(); break;
      case 5: showLineStations();     pause(); break;
      case 6: return;
      default:
        std::cout << "输入无效，请输入数字选项1、2、3、4、5、6\n";
        pause();
    }
  }
}


// ---------- 1) 批量更新 ----------
void Menu::batchUpdateFromCSV() {
  clearScreen();
  std::cout << "--- 批量更新站点状态 ---\n";
  std::string path = readLine(
      "  请输入 CSV 路径（直接回车使用 data/update_station_status.csv）: ");
  if (path.empty()) path = "data/update_station_status.csv";
  bool ok = stationManager_.batchUpdateFromCSV(path);
  if (ok && !stationManager_.saveCurrentToCSV("data/Station.csv"))
    std::cout << "目标文件不存在。\n";
  if (ok)
    std::cout << "  [完成] 批量更新成功。\n";
  else
    std::cout << "  [提示] 批量更新结束，请检查上方错误信息。\n";
}


// ---------- 2) 手工更新单个站点 ----------
void Menu::toggleOneStation() {
  clearScreen();
  std::cout << "--- 手工更新站点状态 ---\n";
  int id = fuzzyPickStation("请输入待修改站点关键词（exit退出）：");
  if (id < 0) return;
  const Station *s = stationManager_.findById(id);
  if (!s) return;
  std::cout << s->name << "," << s->line << "," << s->status << "\n";
  std::cout << "请输入站点状态（开启/关闭）>";
  std::string newSt;
  std::getline(std::cin, newSt);
  if (newSt != "开启" && newSt != "关闭") {
    std::cout << "状态非法，必须为“开启”或“关闭”。\n";
    return;
  }
  if (stationManager_.setStationStatus(s->name, s->line, newSt)) {
    stationManager_.saveCurrentToCSV("data/Station.csv");
    std::cout << "修改站点: " << s->name << " (" << s->line
              << ") -> 状态: " << newSt << "\n";
    std::cout << "1个站点的状态修改完成。\n";
  } else {
    std::cout << "未匹配到对应站点。\n";
  }
}


// ---------- 3) 显示当前关闭站点 ----------
void Menu::showClosedStations() {
  clearScreen();
  std::cout << "--- 当前关闭的站点 ---\n";
  auto closed = stationManager_.getClosedStations();
  if (closed.empty()) {
    std::cout << "所有站点均处于开放状态。\n";
    return;
  }
  for (size_t i = 0; i < closed.size(); ++i) {
    std::cout << "  " << (i + 1) << ". " << closed[i].name
              << " (" << closed[i].line << ") - " << closed[i].status << "\n";
  }
  std::cout << "  共 " << closed.size() << " 个关闭站点。\n";
}


// ---------- 4) 恢复所有站点初始状态 ----------
void Menu::restoreInitial() {
  clearScreen();
  std::cout << "--- 恢复所有站点初始状态 ---\n";
  std::string confirm = readLine("您确定要恢复所有站点的初始状态?（Y/N）");
  if (confirm == "Y" || confirm == "y") {
    stationManager_.restoreInitialStatus();
    stationManager_.saveCurrentToCSV("data/Station.csv");
    std::cout << "已成功恢复所有站点至初始状态。\n";
  }
}


// ---------- 5) 显示线路站点信息（按运行效果图 §3.8.1）----------
void Menu::showLineStations() {
  clearScreen();
  std::cout << "--- 显示线路站点信息 ---\n";
  auto allLines = stationManager_.getAllLines();
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
  if (choice == 0 || (size_t)choice > allLines.size()) {
    std::cout << "  [提示] 线路序号无效，请输入 1 到 "
              << allLines.size() << " 之间的数字。\n";
    return;
  }

  std::string line = allLines[choice - 1];
  auto sts = stationManager_.getStationsByLine(line);
  std::cout << "  " << line << " 共有 " << sts.size() << " 个站点：\n";

  // 对每个站点：判断是否为换乘站（同名存在多条线路 → 是换乘站）
  for (size_t i = 0; i < sts.size(); ++i) {
    const auto &s = sts[i];
    auto same = stationManager_.getStationsByName(s.name);
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
    if (s.status == "关闭")
      std::cout << " [关闭]";
    std::cout << "\n";
  }
}


// ============================================================
//   线路规划子菜单（按运行效果图 §3.8.2）
// ============================================================
void Menu::timePathMenu() {
  while (true) {
    clearScreen();
    std::cout << "-- 所需时间最短路径规划 --\n";
    std::cout << "1. 单条所需时间最短路径\n";
    std::cout << "2. 3条所需时间最短路径\n";
    std::cout << "3. 返回上级菜单\n";
    int ch = readInt("请输入选项编号:");
    switch (ch) {
      case 1: runPathQuery(0); pause(); break;
      case 2: runPathQuery(2); pause(); break;
      case 3: return;
      default:
        std::cout << "输入无效，请输入数字选项1、2、3\n";
        pause();
    }
  }
}

void Menu::transferPathMenu() {
  while (true) {
    clearScreen();
    std::cout << "-- 所需换乘次数最少路径规划 --\n";
    std::cout << "1. 单条换乘次数最少路径\n";
    std::cout << "2. 3条换乘次数最少路径\n";
    std::cout << "3. 返回主菜单\n";
    int ch = readInt("请输入选项编号:");
    switch (ch) {
      case 1: runPathQuery(1); pause(); break;
      case 2: runPathQuery(3); pause(); break;
      case 3: return;
      default:
        std::cout << "输入无效，请输入数字选项1、2、3\n";
        pause();
    }
  }
}


// ---------- 路径查询核心 ----------
void Menu::runPathQuery(int mode) {
  clearScreen();
  const char *titles[] = {"单条所需时间最短路径规划",
                          "单条换乘次数最少路径规划",
                          "3条所需时间最短路径规划",
                          "3条换乘次数最少路径规划"};
  std::cout << "--- " << titles[mode] << " ---\n";

  int sId = fuzzyPickStation("请输入起点站关键词：");
  if (sId < 0) return;
  int eId = fuzzyPickStation("请输入终点站关键词：");
  if (eId < 0) return;
  if (sId == eId) {
    std::cout << "起点和终点相同，无需进行路径规划。\n";
    return;
  }
  const Station *ss = stationManager_.findById(sId);
  const Station *es = stationManager_.findById(eId);
  if (ss && ss->status == "关闭") {
    std::cout << "起点：" << ss->name << "（" << ss->line
              << "）已关闭，无法进行路径规划。\n";
    return;
  }
  if (es && es->status == "关闭") {
    std::cout << "终点：" << es->name << "（" << es->line
              << "）已关闭，无法进行路径规划。\n";
    return;
  }

  if (mode == 0) {
    Path p = pathFinder_.findBestByTime(sId, eId);
    if (!p.valid) { std::cout << "未找到可达路径。\n"; return; }
    std::cout << p.toPrettyString(stationManager_) << "\n";
  } else if (mode == 1) {
    Path p = pathFinder_.findBestByTransfer(sId, eId);
    if (!p.valid) { std::cout << "未找到可达路径。\n"; return; }
    std::cout << p.toPrettyString(stationManager_) << "\n";
  } else if (mode == 2) {
    auto paths = pathFinder_.findKShortestByTime(sId, eId, 3);
    if (paths.empty()) { std::cout << "未找到可达路径。\n"; return; }
    for (size_t i = 0; i < paths.size(); ++i) {
      std::cout << "---- 候选 " << (i + 1) << " ----\n"
                << paths[i].toPrettyString(stationManager_) << "\n";
    }
  } else if (mode == 3) {
    auto paths = pathFinder_.findKShortestByTransfer(sId, eId, 3);
    if (paths.empty()) { std::cout << "未找到可达路径。\n"; return; }
    for (size_t i = 0; i < paths.size(); ++i) {
      std::cout << "---- 候选 " << (i + 1) << " ----\n"
                << paths[i].toPrettyString(stationManager_) << "\n";
    }
  }
}


// ============================================================
//   受关闭站点影响站点分析（按运行效果图 §3.8.1 / §3.8.3）
// ============================================================
void Menu::impactMenu() {
  clearScreen();
  std::cout << "--- 受关闭站点影响站点分析 ---\n";
  int id = fuzzyPickStation("  请输入待分析站点关键字：");
  if (id < 0) return;
  const Station *s = stationManager_.findById(id);
  if (!s) return;

  auto info = pathFinder_.analyzeImpact(s->name, s->line);

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
      const Station *ns = stationManager_.findById(nid);
      std::cout << "  " << (ns ? ns->name : "?")
                << "(" << (ns ? ns->line : "?") << ")";
      if (isolated.count(nid))
        std::cout << " [关闭后将成为无相邻站点]";
      std::cout << "\n";
    }
  }

  std::cout << "\n影响等级：" << info.level << "\n";
}


void Menu::addNewStation() {
  clearScreen();
  std::cout << "--- 添加新站点 ---\n";
  std::string name = readLine("  请输入新站点名: ");
  if (name.empty()) { std::cout << "  [取消] 名称不能为空。\n"; return; }
  std::string line = readLine("  请输入所属线路: ");
  line = normalizeLineName(line);
  std::string st   = readLine("  状态 (开启/关闭，默认开启): ");
  if (st.empty()) st = "开启";
  if (st != "开启" && st != "关闭") {
    std::cout << "  [错误] 状态只能是 开启/关闭。\n"; return;
  }
  int newId = stationManager_.addStation(name, line, st);
  if (newId < 0) {
    std::cout << "  [失败] 已存在同名+同线路的站点。\n";
    return;
  }
  std::cout << "  [成功] 已添加站点 ID=" << newId
            << " (" << name << " / " << line << " / " << st << ")\n";
}
void Menu::removeExistingStation() {
  clearScreen();
  std::cout << "--- 删除站点 ---\n";
  int id = fuzzyPickStation("  请输入要删除的站点关键字：");
  if (id < 0) return;
  const Station *s = stationManager_.findById(id);
  if (!s) return;
  int confirm = readInt("  确认删除? 1=是 0=否 : ", 0);
  if (confirm == 1) {
    if (stationManager_.removeStation(s->name, s->line))
      std::cout << "  [成功] 站点已删除。\n";
    else
      std::cout << "  [失败] 删除失败。\n";
  }
}
void Menu::addNewEdgeInteractive() {
  clearScreen();
  std::cout << "--- 手动添加一条边 ---\n";
  int u = fuzzyPickStation("  请输入起点站: ");
  if (u < 0) return;
  int v = fuzzyPickStation("  请输入终点站: ");
  if (v < 0) return;
  std::string line = readLine("  请输入所属线路: ");
  line = normalizeLineName(line);
  if (line.empty()) { std::cout << "  [取消] 线路不能为空。\n"; return; }
  int t = readInt("  请输入通行时间（分钟，默认 3）: ", 3);
  if (graph_.addEdge(u, v, line, t))
    std::cout << "  [成功] 已添加 " << u << " -> " << v
              << " (" << line << ", " << t << " min)\n";
  else
    std::cout << "  [失败] 起点或终点不存在。\n";
}
