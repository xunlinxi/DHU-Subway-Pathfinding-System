// main_web.cpp - Web 入口，接收命令行参数，输出结构化文本供 Python 解析
#include "graph.h"
#include "pathfinder.h"
#include "station.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

static const std::string STATION_CSV = "data/Station.csv";
static const std::string STATION_INIT = "data/Station_init.csv";
static const std::string EDGE_CSV = "data/Edge.csv";

// Windows argv 按 ANSI 代码页解码，转 UTF-8 避免中文乱码
static std::string ansiToUtf8(const char *ansi) {
  if (!ansi || !*ansi)
    return std::string(ansi ? ansi : "");
#ifdef _WIN32
  int wlen = MultiByteToWideChar(CP_ACP, 0, ansi, -1, nullptr, 0);
  if (wlen <= 0)
    return std::string(ansi);
  std::wstring wstr(wlen, L'\0');
  MultiByteToWideChar(CP_ACP, 0, ansi, -1, &wstr[0], wlen);
  int u8len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0,
                                  nullptr, nullptr);
  if (u8len <= 0)
    return std::string(ansi);
  std::string u8(u8len, '\0');
  WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &u8[0], u8len, nullptr,
                      nullptr);
  if (!u8.empty() && u8.back() == '\0')
    u8.pop_back();
  return u8;
#else
  return std::string(ansi);
#endif
}

// 输出辅助：协议首行 OK/ERR
static void emitOK() { std::cout << "OK\n"; }
static void emitErr(const std::string &msg) {
  std::cout << "ERR\nMESSAGE=" << msg << "\n";
}

// 输出单条路径
static void emitPath(const Path &p, const StationManager &sm) {
  std::cout << "VALID=" << (p.valid ? "1" : "0") << "\n"
            << "TOTAL_TIME=" << p.totalTime << "\n"
            << "TRANSFERS=" << p.transferCnt << "\n"
            << "STOPS=" << p.stopCnt << "\n"
            << "ACTUAL_STOPS=" << p.actualStopCnt << "\n"
            << "PRETTY_BEGIN\n"
            << p.toPrettyString(sm) << "\n"
            << "PRETTY_END\n";
}

// 输出多条路径（K 短路）
static void emitPaths(const std::vector<Path> &paths,
                      const StationManager &sm) {
  std::cout << "COUNT=" << (int)paths.size() << "\n";
  for (size_t i = 0; i < paths.size(); ++i) {
    const Path &p = paths[i];
    std::cout << "PATH_BEGIN\n"
              << "INDEX=" << (i + 1) << "\n"
              << "VALID=" << (p.valid ? "1" : "0") << "\n"
              << "TOTAL_TIME=" << p.totalTime << "\n"
              << "TRANSFERS=" << p.transferCnt << "\n"
              << "STOPS=" << p.stopCnt << "\n"
              << "ACTUAL_STOPS=" << p.actualStopCnt << "\n"
              << "PRETTY_BEGIN\n"
              << p.toPrettyString(sm) << "\n"
              << "PRETTY_END\n"
              << "PATH_END\n";
  }
}

// 按名称+线路定位站点 id
static int findStationId(const StationManager &sm, const std::string &name,
                         const std::string &line) {
  auto sts = sm.getStationsByName(name);
  if (sts.empty())
    return -1;
  if (line.empty())
    return sts[0].id;
  for (auto &s : sts)
    if (s.line == line)
      return s.id;
  return -1;
}

// 单条路径查询（time / transfer）
static int cmdSingleQuery(StationManager &sm, PathFinder &pf,
                          const std::string &start,
                          const std::string &startLine, const std::string &end,
                          const std::string &endLine, bool byTime) {
  int sId = findStationId(sm, start, startLine);
  int eId = findStationId(sm, end, endLine);
  if (sId < 0) {
    emitErr("找不到起点: " + start);
    return 0;
  }
  if (eId < 0) {
    emitErr("找不到终点: " + end);
    return 0;
  }
  if (sId == eId) {
    emitErr("起点和终点相同，无需进行路径规划");
    return 0;
  }
  const Station *ss = sm.findById(sId);
  const Station *es = sm.findById(eId);
  if (ss && ss->status == "关闭") {
    emitErr("起点：" + ss->name + "（" + ss->line +
            "）已关闭，无法进行路径规划");
    return 0;
  }
  if (es && es->status == "关闭") {
    emitErr("终点：" + es->name + "（" + es->line +
            "）已关闭，无法进行路径规划");
    return 0;
  }
  Path p =
      byTime ? pf.findBestByTime(sId, eId) : pf.findBestByTransfer(sId, eId);
  emitOK();
  std::cout << "START_ID=" << sId << "\n"
            << "END_ID=" << eId << "\n"
            << "MODE=" << (byTime ? "time" : "transfer") << "\n";
  emitPath(p, sm);
  return 0;
}

// K 短路查询（time / transfer）
static int cmdKQuery(StationManager &sm, PathFinder &pf,
                     const std::string &start, const std::string &startLine,
                     const std::string &end, const std::string &endLine, int K,
                     bool byTime) {
  int sId = findStationId(sm, start, startLine);
  int eId = findStationId(sm, end, endLine);
  if (sId < 0) {
    emitErr("找不到起点: " + start);
    return 0;
  }
  if (eId < 0) {
    emitErr("找不到终点: " + end);
    return 0;
  }
  if (sId == eId) {
    emitErr("起点和终点相同，无需进行路径规划");
    return 0;
  }
  const Station *ss = sm.findById(sId);
  const Station *es = sm.findById(eId);
  if (ss && ss->status == "关闭") {
    emitErr("起点：" + ss->name + "（" + ss->line +
            "）已关闭，无法进行路径规划");
    return 0;
  }
  if (es && es->status == "关闭") {
    emitErr("终点：" + es->name + "（" + es->line +
            "）已关闭，无法进行路径规划");
    return 0;
  }
  if (K < 1)
    K = 3;
  std::vector<Path> paths = byTime ? pf.findKShortestByTime(sId, eId, K)
                                   : pf.findKShortestByTransfer(sId, eId, K);
  emitOK();
  std::cout << "START_ID=" << sId << "\n"
            << "END_ID=" << eId << "\n"
            << "MODE=" << (byTime ? "time" : "transfer") << "\n"
            << "K=" << K << "\n";
  emitPaths(paths, sm);
  return 0;
}

// 切换站点状态并自动落盘
static int cmdToggle(StationManager &sm, const std::string &name,
                     const std::string &line, const std::string &status) {
  if (status != "开启" && status != "关闭") {
    emitErr("status 必须是 '开启' 或 '关闭'");
    return 0;
  }
  bool ok = sm.setStationStatus(name, line, status);
  if (!ok) {
    emitErr("更新失败, 站点不存在: " + name + " (" + line + ")");
    return 0;
  }
  sm.saveCurrentToCSV(STATION_CSV);
  emitOK();
  std::cout << "NAME=" << name << "\n"
            << "LINE=" << line << "\n"
            << "STATUS=" << status << "\n"
            << "MESSAGE=状态已更新并保存\n";
  return 0;
}

// 站点列表查询命令（全部 / 按线路 / 仅关闭）
static int cmdListAll(const StationManager &sm) {
  emitOK();
  int closedCount = 0;
  std::cout << "COUNT=" << (int)sm.allStations().size() << "\n";
  for (const auto &s : sm.allStations()) {
    std::cout << "ITEM_BEGIN\n"
              << "ID=" << s.id << "\n"
              << "NAME=" << s.name << "\n"
              << "LINE=" << s.line << "\n"
              << "STATUS=" << s.status << "\n"
              << "ITEM_END\n";
    if (s.status == "关闭")
      ++closedCount;
  }
  std::cout << "CLOSED_COUNT=" << closedCount << "\n";
  return 0;
}

static int cmdListLine(const StationManager &sm, const std::string &line) {
  auto sts = sm.getStationsByLine(line);
  emitOK();
  std::cout << "LINE=" << line << "\n"
            << "COUNT=" << (int)sts.size() << "\n";
  for (const auto &s : sts) {
    std::cout << "ITEM_BEGIN\n"
              << "ID=" << s.id << "\n"
              << "NAME=" << s.name << "\n"
              << "LINE=" << s.line << "\n"
              << "STATUS=" << s.status << "\n"
              << "ITEM_END\n";
  }
  return 0;
}

static int cmdListClosed(const StationManager &sm) {
  auto closed = sm.getClosedStations();
  emitOK();
  std::cout << "COUNT=" << (int)closed.size() << "\n";
  for (const auto &s : closed) {
    std::cout << "ITEM_BEGIN\n"
              << "ID=" << s.id << "\n"
              << "NAME=" << s.name << "\n"
              << "LINE=" << s.line << "\n"
              << "STATUS=" << s.status << "\n"
              << "ITEM_END\n";
  }
  return 0;
}

// 恢复初始状态并落盘
static int cmdReset(StationManager &sm) {
  sm.restoreInitialStatusSilent();
  sm.saveCurrentToCSV(STATION_CSV);
  emitOK();
  std::cout << "MESSAGE=已恢复初始状态并保存\n";
  return 0;
}

// 换乘站整体关闭：关闭所有同名站点
static int cmdCloseTransfer(StationManager &sm, const std::string &name) {
  int n = sm.closeTransferStationSilent(name);
  if (n > 0)
    sm.saveCurrentToCSV(STATION_CSV);
  emitOK();
  std::cout << "NAME=" << name << "\n";
  std::cout << "COUNT=" << n << "\n";
  std::cout << "MESSAGE=已关闭 " << n << " 个同名换乘站并保存到 CSV\n";
  return 0;
}

// 线路停运/恢复
static int cmdLineToggle(StationManager &sm, const std::string &line,
                         const std::string &action) {
  int n = 0;
  if (action == "close") {
    n = sm.closeLineStationsSilent(line);
  } else if (action == "open") {
    n = sm.openLineStationsSilent(line);
  } else {
    emitErr("action 必须是 open 或 close");
    return 0;
  }
  if (n > 0)
    sm.saveCurrentToCSV(STATION_CSV);
  emitOK();
  std::cout << "LINE=" << line << "\n";
  std::cout << "ACTION=" << action << "\n";
  std::cout << "COUNT=" << n << "\n";
  std::cout << "MESSAGE=线路 [" << line << "] 已"
            << (action == "close" ? "停运" : "恢复") << " (" << n
            << " 个站点) 并保存到 CSV\n";
  return 0;
}

// 全网停运/恢复
static int cmdNetworkToggle(StationManager &sm, const std::string &action) {
  int n = 0;
  if (action == "close") {
    n = sm.closeAllStationsSilent();
  } else if (action == "open") {
    n = sm.openAllStationsSilent();
  } else {
    emitErr("action 必须是 open 或 close");
    return 0;
  }
  if (n > 0)
    sm.saveCurrentToCSV(STATION_CSV);
  emitOK();
  std::cout << "ACTION=" << action << "\n";
  std::cout << "COUNT=" << n << "\n";
  std::cout << "MESSAGE=全网已" << (action == "close" ? "停运" : "恢复") << " ("
            << n << " 个站点) 并保存到 CSV\n";
  return 0;
}

// 关闭影响分析
static int cmdImpact(PathFinder &pf, StationManager &sm,
                     const std::string &name, const std::string &line) {
  auto info = pf.analyzeImpact(name, line);
  emitOK();
  std::cout << "NAME=" << name << "\n";
  std::cout << "LINE=" << line << "\n";
  std::cout << "AFFECTED_LINES=";
  for (size_t i = 0; i < info.affectedLines.size(); ++i) {
    if (i)
      std::cout << ",";
    std::cout << info.affectedLines[i];
  }
  std::cout << "\n";
  std::cout << "SAME_LINE_ADJ_BEGIN\n";
  for (int nid : info.sameLineAdj) {
    const Station *s = sm.findById(nid);
    std::cout << "ID=" << nid << "\nNAME=" << (s ? s->name : "?")
              << "\nLINE=" << (s ? s->line : "?") << "\n";
  }
  std::cout << "SAME_LINE_ADJ_END\n";
  return 0;
}

// 网络连通性分析（含各分量站点明细）
static int cmdNetwork(PathFinder &pf, const StationManager &sm) {
  auto ninfo = pf.analyzeNetworkConnectivity();
  emitOK();
  std::cout << "OPEN=" << ninfo.totalOpenStations << "\n";
  std::cout << "CLOSED=" << ninfo.totalClosedStations << "\n";
  std::cout << "COMPONENTS=" << ninfo.componentCount << "\n";
  std::cout << "IS_CONNECTED=" << (ninfo.isConnected ? 1 : 0) << "\n";
  std::cout << "COMP_BEGIN\n";
  for (int i = 0; i < ninfo.componentCount; ++i) {
    const auto &comp = ninfo.components[i];
    std::cout << "COMP_ID=" << i << "\n"
              << "COMP_SIZE=" << (int)comp.size() << "\n"
              << "STATIONS_BEGIN\n";
    for (int id : comp) {
      const Station *s = sm.findById(id);
      std::cout << "ID=" << id << "\n"
                << "NAME=" << (s ? s->name : "?") << "\n"
                << "LINE=" << (s ? s->line : "?") << "\n";
    }
    std::cout << "STATIONS_END\n";
  }
  std::cout << "COMP_END\n";
  return 0;
}

// 线路名列表命令
static int cmdListLines(const StationManager &sm) {
  auto lines = sm.getAllLines();
  emitOK();
  std::cout << "COUNT=" << (int)lines.size() << "\n";
  std::cout << "ITEM_BEGIN\n";
  for (const auto &l : lines)
    std::cout << "LINE=" << l << "\n";
  std::cout << "ITEM_END\n";
  return 0;
}

// 批量更新：复用 StationManager，临时重定向 cout 保证协议头干净
static int cmdBatchUpdate(StationManager &sm, const std::string &path) {
  std::string p = path;
  if (p.empty())
    p = "data/update_station_status.csv";
  std::stringstream captured;
  std::streambuf *oldBuf = std::cout.rdbuf(captured.rdbuf());
  int n = sm.batchUpdateFromCSV(p);
  std::cout.rdbuf(oldBuf);
  if (n < 0) {
    emitErr("批量更新失败: 文件无法打开或格式错误 (" + p + ")");
    return 0;
  }
  if (n > 0)
    sm.saveCurrentToCSV(STATION_CSV);
  emitOK();
  std::cout << "PATH=" << p << "\n";
  std::cout << "COUNT=" << n << "\n";
  std::cout << "MESSAGE=批量更新已执行 (" << n << " 条) 并保存到 CSV\n";
  return 0;
}

// 用法提示
static void printUsage() {
  std::cout
      << "USAGE\n"
      << "  main_web query         <起点> <起点线路> <终点> <终点线路>\n"
      << "  main_web qtransfer     <起点> <起点线路> <终点> <终点线路>\n"
      << "  main_web ktime         <起点> <起点线路> <终点> <终点线路> [K=3]\n"
      << "  main_web ktransfer     <起点> <起点线路> <终点> <终点线路> [K=3]\n"
      << "  main_web toggle        <站点名> <线路> <开启|关闭>\n"
      << "  main_web close-transfer<换乘站名>\n"
      << "  main_web line-toggle   <线路> <open|close>\n"
      << "  main_web network-toggle<open|close>\n"
      << "  main_web impact        <站点名> <线路>\n"
      << "  main_web network       (网络连通性分析)\n"
      << "  main_web list_all\n"
      << "  main_web list_line     <线路>\n"
      << "  main_web list_closed\n"
      << "  main_web list_lines    (列出所有线路名)\n"
      << "  main_web batch_update  [path]  (批量更新站点状态)\n"
      << "  main_web reset\n"
      << "  main_web help\n";
}

// 主入口：解析命令分发到各 cmd* 处理函数
int main(int argc, char **argv) {
#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif

  if (argc < 2) {
    printUsage();
    return 1;
  }
  std::vector<std::string> args;
  args.reserve(argc);
  for (int i = 0; i < argc; ++i) {
    args.push_back(ansiToUtf8(argv[i]));
  }
  std::string cmd = args[1];

  if (cmd == "help" || cmd == "--help" || cmd == "-h") {
    printUsage();
    return 0;
  }

  StationManager sm;
  if (!sm.loadFromCSV(STATION_CSV)) {
    emitErr("加载 Station.csv 失败: " + STATION_CSV);
    return 0;
  }
  sm.loadInitFromCSV(STATION_INIT);

  Graph graph(sm);
  if (!graph.build(EDGE_CSV)) {
    emitErr("构建图失败: " + EDGE_CSV);
    return 0;
  }
  PathFinder pf(graph, sm);

  if (cmd == "query" && argc >= 6) {
    return cmdSingleQuery(sm, pf, args[2], args[3], args[4], args[5], true);
  }
  if (cmd == "qtransfer" && argc >= 6) {
    return cmdSingleQuery(sm, pf, args[2], args[3], args[4], args[5], false);
  }
  if (cmd == "ktime" && argc >= 6) {
    int K = (argc >= 7) ? std::atoi(args[6].c_str()) : 3;
    return cmdKQuery(sm, pf, args[2], args[3], args[4], args[5], K, true);
  }
  if (cmd == "ktransfer" && argc >= 6) {
    int K = (argc >= 7) ? std::atoi(args[6].c_str()) : 3;
    return cmdKQuery(sm, pf, args[2], args[3], args[4], args[5], K, false);
  }
  if (cmd == "toggle" && argc >= 5) {
    return cmdToggle(sm, args[2], args[3], args[4]);
  }
  if (cmd == "list_all") {
    return cmdListAll(sm);
  }
  if (cmd == "list_line" && argc >= 3) {
    return cmdListLine(sm, args[2]);
  }
  if (cmd == "list_closed") {
    return cmdListClosed(sm);
  }
  if (cmd == "reset") {
    return cmdReset(sm);
  }
  if (cmd == "batch_update") {
    std::string p = (argc >= 3) ? args[2] : "";
    return cmdBatchUpdate(sm, p);
  }
  if (cmd == "close-transfer" && argc >= 3) {
    return cmdCloseTransfer(sm, args[2]);
  }
  if (cmd == "line-toggle" && argc >= 4) {
    return cmdLineToggle(sm, args[2], args[3]);
  }
  if (cmd == "network-toggle" && argc >= 3) {
    return cmdNetworkToggle(sm, args[2]);
  }
  if (cmd == "impact" && argc >= 4) {
    return cmdImpact(pf, sm, args[2], args[3]);
  }
  if (cmd == "network") {
    return cmdNetwork(pf, sm);
  }
  if (cmd == "list_lines") {
    return cmdListLines(sm);
  }

  emitErr("未知命令或参数不足: " + cmd);
  printUsage();
  return 1;
}

// Unity Build
#ifndef NO_UNITY_BUILD
#include "graph.cpp"
#include "pathfinder.cpp"
#include "station.cpp"
#endif
