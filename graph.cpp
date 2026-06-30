// =============================================================
//  graph.cpp
//  实现 Graph：加载同线边 + 自动建换乘边 + 查询
// =============================================================
#include "graph.h"

// ---------- 内部：按 id 调整 adj_ 大小 ----------
static void ensureSize(std::vector<std::vector<Edge>> &adj, int id) {
  if ((int)adj.size() <= id)
    adj.resize(id + 1);
}

// ---------- 添加一条有向边（避免重复） ----------
static void addDirectedEdge(std::vector<std::vector<Edge>> &adj, int from,
                            int to, const std::string &line, int time) {
  if (from < 0 || to < 0)
    return;
  ensureSize(adj, std::max(from, to));
  for (const auto &e : adj[from]) {
    if (e.to == to && e.line == line)
      return; // 同 (from, to, line) 唯一
  }
  adj[from].push_back({to, line, time});
}

// ---------- 加载 Edge.csv ----------
bool Graph::loadEdgesFromCSV(const std::string &csvPath) {
  std::ifstream fin(csvPath);
  if (!fin.is_open()) {
    std::cerr << "[Graph] 无法打开文件: " << csvPath << std::endl;
    return false;
  }
  std::string line;
  bool first = true;
  int maxId = 0;
  while (std::getline(fin, line)) {
    if (line.empty())
      continue;
    // 复用 StationManager 的 CSV 解析
    auto fields = StationManager::parseCSVLine(line);
    if (first) {
      first = false;
      if (fields.size() >= 1 && fields[0] == "from")
        continue;
    }
    // 格式: from, to, line, direction, time
    if (fields.size() < 5)
      continue;
    int from, to, t;
    try {
      from = std::stoi(fields[0]);
      to = std::stoi(fields[1]);
      t = std::stoi(fields[4]);
    } catch (...) {
      continue;
    }
    std::string ln = fields[2];
    addDirectedEdge(adj_, from, to, ln, t);
    maxId = std::max(maxId, std::max(from, to));
  }
  fin.close();
  ensureSize(adj_, maxId);
  return true;
}

// ---------- 自动建立换乘边 ----------
// 同名站点两两互联（不同线路），权重 = TRANSFER_TIME
// 例如 "人民广场" 出现在 1/2/8 号线，则 3 个站 id 两两连 5 分钟
void Graph::buildTransferEdges() {
  transferPairs_.clear();
  // 1) 按 name 分组
  std::unordered_map<std::string, std::vector<int>> name2ids;
  for (const auto &st : stationMgr_.allStations()) {
    name2ids[st.name].push_back(st.id);
  }
  // 2) 遍历每组，组内两两连
  for (const auto &kv : name2ids) {
    const auto &ids = kv.second;
    if (ids.size() < 2)
      continue; // 非换乘站跳过
    for (size_t i = 0; i < ids.size(); ++i) {
      for (size_t j = i + 1; j < ids.size(); ++j) {
        int a = ids[i], b = ids[j];
        auto key = std::minmax(a, b);
        if (transferPairs_.count(key))
          continue; // 已加过
        transferPairs_.insert(key);
        // 双向 5 分钟
        const Station *sa = stationMgr_.findById(a);
        const Station *sb = stationMgr_.findById(b);
        if (!sa || !sb)
          continue;
        addDirectedEdge(adj_, a, b, sa->line, TRANSFER_TIME);
        addDirectedEdge(adj_, b, a, sb->line, TRANSFER_TIME);
      }
    }
  }
}

// ---------- 一键构建 ----------
bool Graph::build(const std::string &edgeCsvPath) {
  adj_.clear();
  if (!loadEdgesFromCSV(edgeCsvPath))
    return false;
  buildTransferEdges();
  return true;
}

// ---------- 同线邻居 ----------
// 返回与 id 在同一条线路上、由原始 Edge.csv 直接相连的前后站
std::vector<int> Graph::sameLineNeighbors(int id) const {
  std::vector<int> res;
  if (id < 0 || id >= (int)adj_.size())
    return res;
  const Station *me = stationMgr_.findById(id);
  if (!me)
    return res;
  for (const auto &e : adj_[id]) {
    if (e.line == me->line)
      res.push_back(e.to);
  }
  return res;
}

// ---------- 换乘邻居 ----------
std::vector<int> Graph::transferNeighbors(int id) const {
  std::vector<int> res;
  if (id < 0 || id >= (int)adj_.size())
    return res;
  const Station *me = stationMgr_.findById(id);
  if (!me)
    return res;
  for (const auto &e : adj_[id]) {
    if (e.line != me->line)
      res.push_back(e.to);
  }
  return res;
}

// ---------- 站点所属线路集合 ----------
std::vector<std::string> Graph::getLinesOfStation(int id) const {
  std::vector<std::string> res;
  if (id < 0 || id >= (int)adj_.size())
    return res;
  std::set<std::string> uniq;
  for (const auto &e : adj_[id])
    uniq.insert(e.line);
  for (const auto &s : uniq)
    res.push_back(s);
  return res;
}

// ---------- 调试打印 ----------
void Graph::debugPrint() const {
  for (size_t i = 0; i < adj_.size(); ++i) {
    const Station *st = stationMgr_.findById((int)i);
    if (!st)
      continue;
    std::cout << "[" << st->id << " " << st->name << " " << st->line << " "
              << st->status << "] -> ";
    for (const auto &e : adj_[i]) {
      const Station *to = stationMgr_.findById(e.to);
      std::cout << "(" << (to ? to->name : "?") << " " << e.line << " "
                << e.time << "min) ";
    }
    std::cout << "\n";
  }
}
