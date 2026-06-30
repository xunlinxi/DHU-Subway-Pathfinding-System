#include "graph.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <ostream>
#include <unordered_map>

namespace {
void ensureSize(std::vector<std::vector<Edge>> &adjacencyList, int id) {
  if ((int)adjacencyList.size() <= id)
    adjacencyList.resize(id + 1);
}

void addDirectedEdge(std::vector<std::vector<Edge>> &adjacencyList, int from,
                            int to, const std::string &line, int time,
                            const std::string &direction = "") {
  if (from < 0 || to < 0)
    return;
  ensureSize(adjacencyList, std::max(from, to));
  for (const auto &e : adjacencyList[from]) {
    if (e.to == to && e.line == line && e.direction == direction)
      return;
  }
  adjacencyList[from].push_back({to, line, direction, time});
}
} // namespace

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
    line = StationManager::trim(line);
    if (line.empty())
      continue;
    auto fields = StationManager::parseCSVLine(line);
    if (first) {
      first = false;
      if (fields.size() >= 1 && fields[0] == "from")
        continue;
    }
    if (fields.size() != 5) {
      std::cerr << "[Graph] 边数据字段数量错误: " << line << std::endl;
      continue;
    }
    int from, to, t;
    try {
      from = std::stoi(fields[0]);
      to = std::stoi(fields[1]);
      t = std::stoi(fields[4]);
    } catch (...) {
      std::cerr << "[Graph] 非法边记录: " << line << std::endl;
      continue;
    }
    if (!stationManager_.findById(from) || !stationManager_.findById(to)) {
      std::cerr << "[Graph] 边引用了不存在的站点: " << line << std::endl;
      continue;
    }
    std::string ln = fields[2];
    std::string dir = fields[3]; // 运行方向（4号线内/外圈）
    addDirectedEdge(adjacencyList_, from, to, ln, t, dir);
    maxId = std::max(maxId, std::max(from, to));
  }
  fin.close();
  ensureSize(adjacencyList_, maxId);
  return true;
}

void Graph::buildTransferEdges() {
  transferPairs_.clear();
  std::unordered_map<std::string, std::vector<int>> name2ids;
  for (const auto &st : stationManager_.allStations()) {
    name2ids[st.name].push_back(st.id);
  }
  for (const auto &entry : name2ids) {
    const auto &ids = entry.second;
    if (ids.size() < 2)
      continue;
    for (size_t i = 0; i < ids.size(); ++i) {
      for (size_t j = i + 1; j < ids.size(); ++j) {
        int a = ids[i], b = ids[j];
        auto key = std::minmax(a, b);
        if (transferPairs_.count(key))
          continue;
        transferPairs_.insert(key);
        const Station *sa = stationManager_.findById(a);
        const Station *sb = stationManager_.findById(b);
        if (!sa || !sb)
          continue;
        addDirectedEdge(adjacencyList_, a, b, "换乘", kTransferMinutes, "换乘");
        addDirectedEdge(adjacencyList_, b, a, "换乘", kTransferMinutes, "换乘");
      }
    }
  }
}

bool Graph::build(const std::string &edgeCsvPath) {
  adjacencyList_.clear();
  if (!loadEdgesFromCSV(edgeCsvPath))
    return false;
  buildTransferEdges();
  return true;
}

void Graph::clear() {
  adjacencyList_.clear();
  transferPairs_.clear();
}

void Graph::rebuild(const std::string &edgeCsvPath) {
  clear();
  loadEdgesFromCSV(edgeCsvPath);
  buildTransferEdges();
}

bool Graph::addEdge(int fromId, int toId,
                    const std::string &line, int timeMin,
                    const std::string &direction) {
  if (fromId < 0 || toId < 0)
    return false;
  if (!stationManager_.findById(fromId) || !stationManager_.findById(toId))
    return false;
  addDirectedEdge(adjacencyList_, fromId, toId, line, timeMin, direction);
  return true;
}

std::vector<int> Graph::sameLineNeighbors(int id) const {
  std::vector<int> res;
  if (id < 0 || id >= (int)adjacencyList_.size())
    return res;
  const Station *me = stationManager_.findById(id);
  if (!me)
    return res;
  for (const auto &e : adjacencyList_[id]) {
    if (e.line == me->line)
      res.push_back(e.to);
  }
  return res;
}

std::vector<int> Graph::transferNeighbors(int id) const {
  std::vector<int> res;
  if (id < 0 || id >= (int)adjacencyList_.size())
    return res;
  const Station *me = stationManager_.findById(id);
  if (!me)
    return res;
  for (const auto &e : adjacencyList_[id]) {
    if (e.line == "换乘")
      res.push_back(e.to);
  }
  return res;
}

std::vector<std::string> Graph::getLinesOfStation(int id) const {
  std::vector<std::string> res;
  if (id < 0 || id >= (int)adjacencyList_.size())
    return res;
  std::set<std::string> uniq;
  for (const auto &e : adjacencyList_[id])
    uniq.insert(e.line);
  for (const auto &line : uniq)
    res.push_back(line);
  return res;
}

void Graph::debugPrint(std::ostream &output) const {
  for (size_t i = 0; i < adjacencyList_.size(); ++i) {
    const Station *st = stationManager_.findById((int)i);
    if (!st)
      continue;
    output << "[" << st->id << " " << st->name << " " << st->line << " "
           << st->status << "] -> ";
    for (const auto &e : adjacencyList_[i]) {
      const Station *to = stationManager_.findById(e.to);
      output << "(" << (to ? to->name : "?") << " " << e.line << " "
             << e.direction << " " << e.time << "min) ";
    }
    output << "\n";
  }
}
