// =============================================================
//  pathfinder.cpp
//  核心算法实现：Dijkstra / Yen's K 短路 / 影响分析
// =============================================================
#include "pathfinder.h"
#include <algorithm>
#include <climits>
#include <map>
#include <sstream>
#include <stack>

namespace {
bool isTransferEdgeLine(const std::string &line) { return line == "换乘"; }
}

// ============================================================
//                      Path 辅助方法
// ============================================================

// 路径签名：站点序列 + 段线路序列，用于去重
std::string Path::signature() const {
  std::string s;
  s.reserve(nodes.size() * 8);
  for (size_t i = 0; i < nodes.size(); ++i) {
    s += std::to_string(nodes[i]);
    if (i < lines.size())
      s += ":" + lines[i];
    s += ";";
  }
  return s;
}

// 可视化输出：
// 起点[线路]：
// 起点 -> ... -> 换乘站
// 站内换乘至[新线路]
// 换乘站 -> ... -> 终点
std::string Path::toPrettyString(const StationManager &sm) const {
  if (!valid || nodes.empty())
    return "[无效路径]";
  auto nameOf = [&](int id) {
    const Station *s = sm.findById(id);
    return s ? s->name : std::string("?");
  };
  auto lineOf = [&](int id) {
    const Station *s = sm.findById(id);
    return s ? s->line : std::string("?");
  };
  // 获取线路方向标签（仅4号线有）
  auto dirOf = [&](int edgeIdx) -> std::string {
    if (edgeIdx < 0 || edgeIdx >= (int)directions.size())
      return "";
    return directions[edgeIdx];
  };

  std::vector<std::string> nodeNames;
  std::vector<std::string> nodeLines;
  nodeNames.reserve(nodes.size());
  nodeLines.reserve(nodes.size());
  for (int id : nodes) {
    nodeNames.push_back(nameOf(id));
    nodeLines.push_back(lineOf(id));
  }

  std::string firstLine = nodeLines[0];
  std::string firstDir = dirOf(0);

  if (nodes.size() == 1) {
    std::ostringstream oss;
    oss << nodeNames[0] << "[" << firstLine << "]：\n";
    oss << nodeNames[0];
    oss << "\n  总耗时: 0 分钟 | 换乘: 0 次 | 途经: 1 站 | 实际经过: 1 站";
    return oss.str();
  }

  std::ostringstream oss;
  auto appendSegment = [&](int startIdx, int endIdx) {
    if (startIdx > endIdx)
      return;
    bool first = true;
    std::string prevName;
    for (int i = startIdx; i <= endIdx; ++i) {
      if (i > startIdx && nodeNames[i] == prevName)
        continue;
      if (!first)
        oss << " -> ";
      oss << nodeNames[i];
      first = false;
      prevName = nodeNames[i];
    }
  };

  oss << nodeNames[0] << "[" << firstLine;
  if (!firstDir.empty())
    oss << " (" << firstDir << ")";
  oss << "]：\n";

  int segmentStart = 0;
  for (int i = 1; i < (int)nodes.size(); ++i) {
    if (nodeLines[i] == nodeLines[i - 1])
      continue;
    appendSegment(segmentStart, i - 1);
    std::string d = dirOf(i);
    oss << "\n站内换乘至[" << nodeLines[i];
    if (!d.empty())
      oss << " (" << d << ")";
    oss << "]\n";
    segmentStart = i;
  }
  appendSegment(segmentStart, (int)nodes.size() - 1);
  oss << "\n  总耗时: " << totalTime << " 分钟 | 换乘: " << transferCnt
      << " 次 | 途经: " << stopCnt << " 站 | 实际经过: "
      << actualStopCnt << " 站";
  return oss.str();
}

// 根据 nodes/lines 重新计算统计
void PathFinder::finalizeStats(Path &p, const Graph &g) {
  p.totalTime = 0;
  p.transferCnt = 0;
  p.stopCnt = (int)p.nodes.size();
  if (p.nodes.size() <= 1) {
    p.actualStopCnt = p.stopCnt;
    return;
  }
  for (int i = 0; i + 1 < (int)p.nodes.size(); ++i) {
    int u = p.nodes[i], v = p.nodes[i + 1];
    for (const auto &e : g.neighbors(u)) {
      if (e.to == v && e.line == p.lines[i]) {
        p.totalTime += e.time;
        break;
      }
    }
    if (i >= 1 && !isTransferEdgeLine(p.lines[i]) &&
        p.lines[i] != p.lines[i - 1])
      ++p.transferCnt;
  }
  // 实际经过站数 = 总节点数 - 因换乘拆分多算的节点
  // 每次换乘会把同一个"换乘站"拆成两个 id 节点；因此要去掉冗余
  p.actualStopCnt = p.stopCnt - p.transferCnt;
}

// ============================================================
//                    Dijkstra 核心
// ============================================================
//
// 设计要点：
//   状态 = (stationId, arrivedLine)，因为同一站位于不同线路时
//   "已发生的换乘次数"不同；dist[state] = (主键, 副键)。
//   - OptGoal::TIME     → 主键=time, 副键=transfer
//   - OptGoal::TRANSFER → 主键=transfer, 副键=time
//   剪枝：
//     (1) 关闭站点直接跳过
//     (2) 同一 (id, line) 状态，更优才入队
//     (3) constrained 模式可禁用特定边 / 节点
// ============================================================

namespace {
struct StateKey {
  int id;
  std::string line;
  bool operator<(const StateKey &o) const {
    if (id != o.id)
      return id < o.id;
    return line < o.line;
  }
};
struct PQState {
  int cost1, cost2;
  int id;
  std::string line;
  bool operator>(const PQState &o) const {
    if (cost1 != o.cost1)
      return cost1 > o.cost1;
    if (cost2 != o.cost2)
      return cost2 > o.cost2;
    if (id != o.id)
      return id > o.id;
    return line > o.line;
  }
};
} // namespace

static Path dijkstraCore(int startId, int endId, OptGoal goal,
                         const Graph &graph, const StationManager &sm,
                         const std::unordered_set<int> *blockedNodes = nullptr,
                         int bannedFrom = -1, int bannedTo = -1,
                         const std::string &bannedLine = "") {
  const auto &adj = graph.adjList();
  const Station *startSt = sm.findById(startId);
  auto stationLineOf = [&](int id) {
    const Station *s = sm.findById(id);
    return s ? s->line : std::string("?");
  };
  if (!startSt || sm.isClosed(startId))
    return Path{};

  std::map<StateKey, std::pair<int, int>> dist;
  std::map<StateKey, StateKey> prev;

  dist[{startId, startSt->line}] = {0, 0};
  std::priority_queue<PQState, std::vector<PQState>, std::greater<PQState>> pq;
  pq.push({0, 0, startId, startSt->line});

  while (!pq.empty()) {
    PQState cur = pq.top();
    pq.pop();
    StateKey curKey{cur.id, cur.line};
    auto it = dist.find(curKey);
    if (it == dist.end())
      continue;
    if (cur.cost1 > it->second.first ||
        (cur.cost1 == it->second.first && cur.cost2 > it->second.second)) {
      continue; // 过期
    }
    if (cur.id == endId) {
      // 回溯路径
      Path p;
      p.valid = true;
      std::vector<int> revNodes;
      std::vector<std::string> revLines;
      StateKey k = curKey;
      while (!(k.id == startId && k.line == startSt->line)) {
        revNodes.push_back(k.id);
        revLines.push_back(k.line);
        auto pit = prev.find(k);
        if (pit == prev.end())
          break;
        k = pit->second;
      }
      revNodes.push_back(startId);
      revLines.push_back(startSt->line);
      for (int i = (int)revNodes.size() - 1; i >= 0; --i)
        p.nodes.push_back(revNodes[i]);
      int sz = (int)revNodes.size();
      for (int i = 0; i < sz - 1; ++i)
        p.lines.push_back(revLines[sz - 2 - i]);
      // 统计
      PathFinder::finalizeStats(p, graph);
      // 填充方向信息
      for (int i = 0; i + 1 < (int)p.nodes.size(); ++i) {
        int u = p.nodes[i], v = p.nodes[i + 1];
        for (const auto &e : adj[u]) {
          if (e.to == v && e.line == p.lines[i]) {
            p.directions.push_back(e.direction);
            break;
          }
        }
      }
      // 用优先队列中保存的 cost 覆盖（与邻接表重算结果应一致；这里取 PQ
      // 状态更稳）
      if (goal == OptGoal::TIME) {
        p.totalTime = cur.cost1;
        p.transferCnt = cur.cost2;
      } else {
        p.transferCnt = cur.cost1;
        p.totalTime = cur.cost2;
      }
      return p;
    }

    if (cur.id >= (int)adj.size())
      continue;
    for (const auto &e : adj[cur.id]) {
      if (cur.id == bannedFrom && e.to == bannedTo && e.line == bannedLine)
        continue;
      if (sm.isClosed(e.to))
        continue;
      if (blockedNodes && blockedNodes->count(e.to) && e.to != endId)
        continue;

      std::string nextLine =
          isTransferEdgeLine(e.line) ? stationLineOf(e.to) : e.line;
      int addTrans = (cur.line != nextLine) ? 1 : 0;
      int newC1, newC2;
      if (goal == OptGoal::TIME) {
        newC1 = cur.cost1 + e.time;
        newC2 = cur.cost2 + addTrans;
      } else {
        newC1 = cur.cost1 + addTrans;
        newC2 = cur.cost2 + e.time;
      }
      StateKey nk{e.to, nextLine};
      auto dit = dist.find(nk);
      if (dit == dist.end() || newC1 < dit->second.first ||
          (newC1 == dit->second.first && newC2 < dit->second.second)) {
        dist[nk] = {newC1, newC2};
        prev[nk] = curKey;
        pq.push({newC1, newC2, e.to, nextLine});
      }
    }
  }
  return Path{};
}

// ---------- 对外封装 ----------
Path PathFinder::dijkstraFull(int startId, int endId, OptGoal goal) {
  return dijkstraCore(startId, endId, goal, graph_, sm_);
}
Path PathFinder::dijkstraConstrained(
    int startId, int endId, OptGoal goal,
    const std::unordered_set<int> &blockedNodes, int bannedFrom, int bannedTo,
    const std::string &bannedLine) {
  return dijkstraCore(startId, endId, goal, graph_, sm_, &blockedNodes,
                      bannedFrom, bannedTo, bannedLine);
}

// ============================================================
//                     1) 单条最优
// ============================================================
Path PathFinder::findBestByTime(int startId, int endId) {
  return dijkstraFull(startId, endId, OptGoal::TIME);
}
Path PathFinder::findBestByTransfer(int startId, int endId) {
  return dijkstraFull(startId, endId, OptGoal::TRANSFER);
}

// ============================================================
//        2) K 短路（Yen 算法）
// ============================================================
//
//   步骤：
//   1) 跑一次完整 Dijkstra 得到 P1
//   2) 对 P1 中每个"偏离点" i：
//        a) 取 P1[0..i] 为 root path
//        b) blocked = P1[0..i-1]（不允许 spur path 绕回 root 前段）
//        c) 禁用 P1 在 i 处的边 (P1[i] -> P1[i+1], P1.lines[i])
//        d) 从 P1[i] 出发跑 constrained Dijkstra → spur path
//        e) total = root + spur 合成
//   3) 把所有候选按目标函数排序，依次取出前 K 个
//   4) 用 signature() 去重
// ============================================================
namespace {
struct PathComparator {
  OptGoal goal;
  bool operator()(const Path &a, const Path &b) const {
    if (goal == OptGoal::TIME) {
      if (a.totalTime != b.totalTime)
        return a.totalTime < b.totalTime;
      if (a.transferCnt != b.transferCnt)
        return a.transferCnt < b.transferCnt;
      return a.stopCnt < b.stopCnt;
    } else {
      if (a.transferCnt != b.transferCnt)
        return a.transferCnt < b.transferCnt;
      if (a.totalTime != b.totalTime)
        return a.totalTime < b.totalTime;
      return a.stopCnt < b.stopCnt;
    }
  }
};
} // namespace

static std::vector<Path> kShortestYen(int startId, int endId, int K,
                                      OptGoal goal, PathFinder &pf) {
  std::vector<Path> result;
  Path p1 = pf.dijkstraFull(startId, endId, goal);
  if (!p1.valid)
    return result;
  result.push_back(p1);

  std::unordered_set<std::string> seen;
  seen.insert(p1.signature());
  std::vector<Path> candidates;

  for (int ki = 1; ki < K; ++ki) {
    const Path &prev = result[ki - 1];
    for (int i = 0; i + 1 < (int)prev.nodes.size(); ++i) {
      int spurNode = prev.nodes[i];
      int nextNode = prev.nodes[i + 1];
      std::string sp = prev.lines[i];

      std::unordered_set<int> blocked;
      for (int j = 0; j < i; ++j)
        blocked.insert(prev.nodes[j]);

      Path spur = pf.dijkstraConstrained(spurNode, endId, goal, blocked,
                                         spurNode, nextNode, sp);
      if (!spur.valid)
        continue;

      // 拼接：root 前 i 站 + spur 完整路径
      Path total;
      for (int j = 0; j < i; ++j)
        total.nodes.push_back(prev.nodes[j]);
      for (int j = 0; j < i; ++j)
        total.lines.push_back(prev.lines[j]);
      for (int j = 0; j < (int)spur.nodes.size(); ++j)
        total.nodes.push_back(spur.nodes[j]);
      for (int j = 0; j < (int)spur.lines.size(); ++j)
        total.lines.push_back(spur.lines[j]);
      // 方向信息
      for (int j = 0; j < i && j < (int)prev.directions.size(); ++j)
        total.directions.push_back(prev.directions[j]);
      for (int j = 0; j < (int)spur.directions.size(); ++j)
        total.directions.push_back(spur.directions[j]);
      PathFinder::finalizeStats(total, pf.graph());
      total.valid = true;

      std::string sig = total.signature();
      if (seen.count(sig))
        continue;
      bool dup = false;
      for (const auto &c : candidates) {
        if (c.signature() == sig) {
          dup = true;
          break;
        }
      }
      if (dup)
        continue;
      seen.insert(sig);
      candidates.push_back(total);
    }
    if (candidates.empty())
      break;
    PathComparator cmp{goal};
    std::sort(candidates.begin(), candidates.end(), cmp);
    result.push_back(candidates.front());
    candidates.erase(candidates.begin());
  }
  return result;
}

std::vector<Path> PathFinder::findKShortestByTime(int startId, int endId,
                                                  int K) {
  return kShortestYen(startId, endId, K, OptGoal::TIME, *this);
}
std::vector<Path> PathFinder::findKShortestByTransfer(int startId, int endId,
                                                      int K) {
  return kShortestYen(startId, endId, K, OptGoal::TRANSFER, *this);
}

// ============================================================
//                  3) 受影响区域分析
// ============================================================
PathFinder::ImpactInfo PathFinder::analyzeImpact(const std::string &name,
                                                 const std::string &line) {
  ImpactInfo info;
  info.name = name;
  info.line = line;
  auto stations = sm_.getStationsByName(name);
  int targetId = -1;
  for (const auto &s : stations) {
    if (s.line == line) {
      targetId = s.id;
      break;
    }
  }
  if (targetId < 0)
    return info;
  info.sameLineAdj = graph_.sameLineNeighbors(targetId);
  {
    std::vector<int> dedupAdj;
    std::unordered_set<int> seenAdj;
    for (int sid : info.sameLineAdj) {
      if (seenAdj.insert(sid).second)
        dedupAdj.push_back(sid);
    }
    info.sameLineAdj.swap(dedupAdj);
  }

  // 找出"被关后变成孤岛"的邻站
  for (int sid : info.sameLineAdj) {
    auto nbrs = graph_.sameLineNeighbors(sid);
    std::vector<int> uniqNbrs;
    std::unordered_set<int> seenNbrs;
    for (int nid : nbrs) {
      if (seenNbrs.insert(nid).second)
        uniqNbrs.push_back(nid);
    }
    bool onlyTarget = (uniqNbrs.size() == 1 && uniqNbrs[0] == targetId);
    if (onlyTarget)
      info.noAdj.push_back(sid);
  }
  {
    std::vector<int> dedupNoAdj;
    std::unordered_set<int> seenNoAdj;
    for (int sid : info.noAdj) {
      if (seenNoAdj.insert(sid).second)
        dedupNoAdj.push_back(sid);
    }
    info.noAdj.swap(dedupNoAdj);
  }
  info.affectedLines.push_back(line);

  int transferLineCnt = (int)stations.size();
  int sameLineCnt = (int)info.sameLineAdj.size();
  if (transferLineCnt >= 3 || sameLineCnt >= 3)
    info.level = "高";
  else if (transferLineCnt == 2 || sameLineCnt == 2)
    info.level = "中";
  else
    info.level = "低";
  return info;
}

// ============================================================
//                  4) 网络连通性分析 (DFS)
// ============================================================
PathFinder::NetworkInfo PathFinder::analyzeNetworkConnectivity() {
  NetworkInfo ninfo;
  ninfo.totalOpenStations = 0;
  ninfo.totalClosedStations = 0;

  // 收集所有开放站点的 id
  std::unordered_set<int> openIds;
  for (const auto &st : sm_.allStations()) {
    if (st.status == "开启") {
      openIds.insert(st.id);
      ++ninfo.totalOpenStations;
    } else {
      ++ninfo.totalClosedStations;
    }
  }

  if (openIds.empty()) {
    ninfo.componentCount = 0;
    ninfo.isConnected = false;
    return ninfo;
  }

  // DFS 遍历连通分量
  std::unordered_set<int> visited;
  const auto &adj = graph_.adjList();

  std::function<void(int, std::vector<int> &)> dfs =
      [&](int cur, std::vector<int> &comp) {
        visited.insert(cur);
        comp.push_back(cur);
        if (cur >= (int)adj.size()) return;
        for (const auto &e : adj[cur]) {
          if (sm_.isClosed(e.to)) continue;        // 跳过关闭站点
          if (e.line == "换乘") continue;           // 换乘边不用于连通性（由同线边传递）
          if (visited.count(e.to)) continue;
          dfs(e.to, comp);
        }
      };

  // 对开放站点也遍历换乘邻居（保证跨线路连通性也被计算）
  auto dfsFull = [&](int cur, std::vector<int> &comp) {
    std::stack<int> stk;
    stk.push(cur);
    while (!stk.empty()) {
      int u = stk.top();
      stk.pop();
      if (visited.count(u)) continue;
      visited.insert(u);
      comp.push_back(u);
      if (u >= (int)adj.size()) continue;
      for (const auto &e : adj[u]) {
        if (sm_.isClosed(e.to)) continue;
        if (visited.count(e.to)) continue;
        stk.push(e.to);
      }
    }
  };

  std::vector<int> allOpen;
  for (int id : openIds) allOpen.push_back(id);
  std::sort(allOpen.begin(), allOpen.end());

  for (int id : allOpen) {
    if (visited.count(id)) continue;
    std::vector<int> comp;
    dfsFull(id, comp);
    std::sort(comp.begin(), comp.end());
    ninfo.components.push_back(comp);
  }

  ninfo.componentCount = (int)ninfo.components.size();
  ninfo.isConnected = (ninfo.componentCount == 1);

  return ninfo;
}
