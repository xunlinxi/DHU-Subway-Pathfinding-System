// =============================================================
//  station.cpp
//  实现 StationManager 全部接口
// =============================================================
#include "station.h"
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>


// ---------- 静态工具：去除 UTF-8 BOM、首尾空白 ----------
std::string StationManager::trim(const std::string &s) {
  size_t b = 0, e = s.size();
  // 跳过 UTF-8 BOM
  if (e >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB &&
      (unsigned char)s[2] == 0xBF) {
    b = 3;
  }
  // 跳过首部 ASCII 空白（含全角空格的高位字节也视为空白）
  auto isSpace = [](unsigned char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0xA0;
  };
  while (b < e && isSpace((unsigned char)s[b]))
    ++b;
  while (e > b && isSpace((unsigned char)s[e - 1]))
    --e;
  return s.substr(b, e - b);
}

// ---------- 静态工具：解析一行 CSV ----------
// 支持双引号包裹、半角逗号分隔；题目数据不复杂，但保留通用性
std::vector<std::string> StationManager::parseCSVLine(const std::string &line) {
  std::vector<std::string> fields;
  std::string cur;
  bool inQuote = false;
  for (size_t i = 0; i < line.size(); ++i) {
    char c = line[i];
    if (c == '"') {
      inQuote = !inQuote;
    } else if (c == ',' && !inQuote) {
      fields.push_back(trim(cur));
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  fields.push_back(trim(cur));
  return fields;
}

// ---------- 内部：建立索引 ----------
void rebuildIndexHelper(
    std::vector<Station> &stations, std::unordered_map<int, size_t> &id2idx,
    std::unordered_map<std::string, std::vector<size_t>> &name2idx) {
  id2idx.clear();
  name2idx.clear();
  for (size_t i = 0; i < stations.size(); ++i) {
    id2idx[stations[i].id] = i;
    name2idx[stations[i].name].push_back(i);
  }
}

// ---------- 加载 Station.csv ----------
bool StationManager::loadFromCSV(const std::string &csvPath) {
  std::ifstream fin(csvPath);
  if (!fin.is_open()) {
    std::cerr << "[Station] 无法打开文件: " << csvPath << std::endl;
    return false;
  }
  std::string line;
  bool first = true;
  stations_.clear();
  while (std::getline(fin, line)) {
    if (line.empty())
      continue;
    auto fields = parseCSVLine(line);
    if (first) { // 跳过表头
      first = false;
      if (fields.size() >= 1 && fields[0] == "id")
        continue;
    }
    if (fields.size() < 4)
      continue;
    Station st;
    try {
      st.id = std::stoi(fields[0]);
    } catch (...) {
      continue;
    }
    st.name = fields[1];
    st.line = fields[2];
    st.status = fields[3];
    stations_.push_back(st);
  }
  fin.close();
  rebuildIndexHelper(stations_, id2idx_, name2idx_);
  return true;
}

// ---------- 加载 Station_init.csv ----------
bool StationManager::loadInitFromCSV(const std::string &csvPath) {
  std::ifstream fin(csvPath);
  if (!fin.is_open()) {
    std::cerr << "[Station] 无法打开初始化文件: " << csvPath << std::endl;
    return false;
  }
  std::string line;
  bool first = true;
  initSnapshot_.clear();
  while (std::getline(fin, line)) {
    if (line.empty())
      continue;
    auto fields = parseCSVLine(line);
    if (first) {
      first = false;
      if (fields.size() >= 1 && fields[0] == "id")
        continue;
    }
    if (fields.size() < 4)
      continue;
    Station st;
    try {
      st.id = std::stoi(fields[0]);
    } catch (...) {
      continue;
    }
    st.name = fields[1];
    st.line = fields[2];
    st.status = fields[3];
    initSnapshot_.push_back(st);
  }
  fin.close();
  return true;
}

// ---------- 批量更新状态 ----------
bool StationManager::batchUpdateFromCSV(const std::string &csvPath) {
  std::ifstream fin(csvPath);
  if (!fin.is_open()) {
    std::cerr << "[Station] 无法打开批量更新文件: " << csvPath << std::endl;
    return false;
  }
  std::string line;
  bool first = true;
  int success = 0, fail = 0;
  while (std::getline(fin, line)) {
    if (line.empty())
      continue;
    auto fields = parseCSVLine(line);
    if (first) {
      first = false;
      if (fields.size() >= 1 && fields[0] == "name")
        continue;
    }
    if (fields.size() < 3)
      continue;
    std::string name = fields[0];
    std::string ln = fields[1];
    std::string st = fields[2];
    if (setStationStatus(name, ln, st))
      ++success;
    else
      ++fail;
  }
  fin.close();
  std::cout << "[批量更新] 成功 " << success << " 条，失败 " << fail
            << " 条。\n";
  return true;
}

// ---------- 模糊搜索 ----------
std::vector<Station>
StationManager::fuzzySearch(const std::string &keyword) const {
  std::vector<Station> res;
  if (keyword.empty())
    return res;
  for (const auto &st : stations_) {
    if (st.name.find(keyword) != std::string::npos) {
      res.push_back(st);
    }
  }
  return res;
}

// ---------- 全名匹配 ----------
std::vector<Station>
StationManager::getStationsByName(const std::string &name) const {
  std::vector<Station> res;
  auto it = name2idx_.find(name);
  if (it != name2idx_.end()) {
    for (size_t i : it->second)
      res.push_back(stations_[i]);
  }
  return res;
}

// ---------- id 查询 ----------
const Station *StationManager::findById(int id) const {
  auto it = id2idx_.find(id);
  if (it == id2idx_.end())
    return nullptr;
  return &stations_[it->second];
}

// ---------- 关闭站点列表 ----------
std::vector<Station> StationManager::getClosedStations() const {
  std::vector<Station> res;
  for (const auto &st : stations_) {
    if (st.status == "关闭")
      res.push_back(st);
  }
  return res;
}

// ---------- 同线站点列表 ----------
std::vector<Station>
StationManager::getStationsByLine(const std::string &line) const {
  std::vector<Station> res;
  for (const auto &st : stations_) {
    if (st.line == line)
      res.push_back(st);
  }
  return res;
}

// ---------- 切换单个站点状态 ----------
bool StationManager::setStationStatus(const std::string &name,
                                      const std::string &line,
                                      const std::string &newStatus) {
  auto it = name2idx_.find(name);
  if (it == name2idx_.end())
    return false;
  for (size_t i : it->second) {
    if (stations_[i].line == line) {
      stations_[i].status = newStatus;
      // 同步到初始快照：手动改的不动 initSnapshot_，避免被恢复覆盖
      return true;
    }
  }
  return false;
}

// ---------- 恢复初始状态 ----------
void StationManager::restoreInitialStatus() {
  if (initSnapshot_.empty()) {
    std::cout << "[警告] 未加载 Station_init.csv，无法恢复。\n";
    return;
  }
  // 把 initSnapshot_ 的 status 写回 stations_
  for (const auto &init : initSnapshot_) {
    auto it = id2idx_.find(init.id);
    if (it != id2idx_.end()) {
      stations_[it->second].status = init.status;
    }
  }
  std::cout << "[恢复] 已将所有站点状态恢复为初始快照。\n";
}

// ---------- 保存当前状态到 CSV ----------
bool StationManager::saveCurrentToCSV(const std::string &csvPath) const {
  std::ofstream fout(csvPath);
  if (!fout.is_open()) {
    std::cerr << "[Station] 无法写入文件: " << csvPath << std::endl;
    return false;
  }
  fout << "id,name,line,status\n";
  for (const auto &st : stations_) {
    fout << st.id << "," << st.name << "," << st.line << "," << st.status
         << "\n";
  }
  fout.close();
  return true;
}

// ---------- 是否关闭 ----------
bool StationManager::isClosed(int id) const {
  auto it = id2idx_.find(id);
  if (it == id2idx_.end())
    return true; // 不存在视为不可用
  return stations_[it->second].status == "关闭";
}
