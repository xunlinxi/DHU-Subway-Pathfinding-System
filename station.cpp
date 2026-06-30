// =============================================================
//  station.cpp
//  实现 StationManager 全部接口
// =============================================================
#include "station.h"
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>


// 重建 id2idx_ / name2idx_ 索引（用于 load / addStation / removeStation 后同步）
// 前向声明：在所有成员函数之前定义，避免在 loadFromCSV 等函数中引用不到
static void rebuildStationsIndex(
    const std::vector<Station>& stations,
    std::unordered_map<int, size_t>& id2idx,
    std::unordered_map<std::string, std::vector<size_t>>& name2idx) {
  id2idx.clear();
  name2idx.clear();
  for (size_t i = 0; i < stations.size(); ++i) {
    id2idx[stations[i].id] = i;
    name2idx[stations[i].name].push_back(i);
  }
}


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
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
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
  rebuildStationsIndex(stations_, id2idx_, name2idx_);
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
int StationManager::batchUpdateFromCSV(const std::string &csvPath) {
  std::ifstream fin(csvPath);
  if (!fin.is_open()) {
    std::cerr << "[Station] 无法打开批量更新文件: " << csvPath << std::endl;
    return -1;
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
      if (!fields.empty() &&
          (fields[0] == "name" || fields[0] == "id"
           || fields[0] == "站点名称" || fields[0] == "站点ID")) {
        // 跳过中英文表头
        continue;
      }
    }
    if (fields.size() == 2) {
      int id = -1;
      try {
        id = std::stoi(fields[0]);
      } catch (...) {
        ++fail;
        continue;
      }
      const Station *s = findById(id);
      if (!s) {
        ++fail;
        continue;
      }
      if (fields[1] != "开启" && fields[1] != "关闭") {
        ++fail;
        continue;
      }
      if (setStationStatus(s->name, s->line, fields[1]))
        ++success;
      else
        ++fail;
      continue;
    }
    if (fields.size() >= 3) {
      std::string name = fields[0];
      std::string ln = fields[1];
      std::string st = fields[2];
      if (st != "开启" && st != "关闭") {
        ++fail;
        continue;
      }
      if (setStationStatus(name, ln, st))
        ++success;
      else
        ++fail;
      continue;
    }
    ++fail;
  }
  fin.close();
  std::cout << "[批量更新] 成功 " << success << " 条，失败 " << fail
            << " 条。\n";
  return success;
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

// ---------- 当前所有线路列表 ----------
std::vector<std::string> StationManager::getAllLines() const {
  std::vector<std::string> lines;
  std::unordered_set<std::string> seen;
  for (const auto &st : stations_) {
    if (seen.insert(st.line).second)
      lines.push_back(st.line);
  }

  auto lineOrder = [](const std::string &line) {
    int value = 0;
    bool hasDigit = false;
    for (unsigned char ch : line) {
      if (std::isdigit(ch)) {
        hasDigit = true;
        value = value * 10 + (ch - '0');
      } else if (hasDigit) {
        break;
      }
    }
    return hasDigit ? value : -1;
  };

  std::sort(lines.begin(), lines.end(),
            [&](const std::string &a, const std::string &b) {
              int aOrder = lineOrder(a);
              int bOrder = lineOrder(b);
              bool aNumbered = aOrder >= 0;
              bool bNumbered = bOrder >= 0;
              if (aNumbered != bNumbered)
                return aNumbered > bNumbered;
              if (aNumbered && aOrder != bOrder)
                return aOrder < bOrder;
              return a < b;
            });
  return lines;
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
    if (it == id2idx_.end()) return true;   // 不存在视为不可用
    return stations_[it->second].status == "关闭";
}

// ============================================================
//    §3.3 规范命名: showClosedStations / showStationsByLine
// ============================================================
void StationManager::showClosedStations(std::ostream& os) const {
    auto closed = getClosedStations();
    os << "  ==== 当前关闭的站点 ====\n";
    if (closed.empty()) {
        os << "  （无）\n";
        return;
    }
    os << "  " << std::left
       << std::setw(8)  << "ID"
       << std::setw(20) << "站名"
       << std::setw(10) << "线路"
       << std::setw(8)  << "状态" << "\n";
    os << "  " << std::string(46, '-') << "\n";
    for (const auto& s : closed) {
        os << "  " << std::left
           << std::setw(8)  << s.id
           << std::setw(20) << s.name
           << std::setw(10) << s.line
           << std::setw(8)  << s.status << "\n";
    }
    os << "  共 " << closed.size() << " 个关闭站点。\n";
}

void StationManager::showStationsByLine(const std::string& line, std::ostream& os) const {
    auto sts = getStationsByLine(line);
    os << "  ==== 线路 [" << line << "] 的所有站点 ====\n";
    if (sts.empty()) {
        os << "  （无）\n";
        return;
    }
    os << "  " << std::left
       << std::setw(8)  << "ID"
       << std::setw(20) << "站名"
       << std::setw(10) << "线路"
       << std::setw(8)  << "状态" << "\n";
    os << "  " << std::string(46, '-') << "\n";
    for (const auto& s : sts) {
        os << "  " << std::left
           << std::setw(8)  << s.id
           << std::setw(20) << s.name
           << std::setw(10) << s.line
           << std::setw(8)  << s.status << "\n";
    }
    os << "  共 " << sts.size() << " 个站点。\n";
}

// ============================================================
//    §3.3 建站管理（可选加分）
// ============================================================
int StationManager::nextStationId() const {
    int mx = 0;
    for (const auto& s : stations_) if (s.id > mx) mx = s.id;
    return mx + 1;
}

int StationManager::addStation(const std::string& name,
                               const std::string& line,
                               const std::string& status) {
    // 1) 重名 + 同线路 视为已存在
    auto same = getStationsByName(name);
    for (const auto& s : same) {
        if (s.line == line) return -1;
    }
    // 2) 分配新 id 并入库
    Station st;
    st.id     = nextStationId();
    st.name   = name;
    st.line   = line;
    st.status = status;
    stations_.push_back(st);
    // 3) 重建索引
    rebuildStationsIndex(stations_, id2idx_, name2idx_);
    return st.id;
}

bool StationManager::removeStation(const std::string& name, const std::string& line) {
    auto it = name2idx_.find(name);
    if (it == name2idx_.end()) return false;
    // 找到匹配 line 的下标
    std::vector<size_t> toErase;
    for (size_t idx : it->second) {
        if (stations_[idx].line == line) toErase.push_back(idx);
    }
    if (toErase.empty()) return false;
    // 从大到小删除
    std::sort(toErase.rbegin(), toErase.rend());
    for (size_t idx : toErase) {
        stations_.erase(stations_.begin() + idx);
    }
    rebuildStationsIndex(stations_, id2idx_, name2idx_);
    return true;
}

// ============================================================
//    运营管理扩展：换乘站整体关闭
// ============================================================
bool StationManager::isTransferStation(const std::string &name) const {
  auto it = name2idx_.find(name);
  if (it == name2idx_.end()) return false;
  return it->second.size() > 1;
}

int StationManager::closeTransferStation(const std::string &name) {
  auto it = name2idx_.find(name);
  if (it == name2idx_.end()) {
    std::cout << "[换乘站关闭] 未找到站点 \"" << name << "\"。\n";
    return 0;
  }
  if (it->second.size() <= 1) {
    std::cout << "[换乘站关闭] \"" << name << "\" 不是换乘站，仅有 1 条线路。\n";
    return 0;
  }
  int cnt = 0;
  for (size_t idx : it->second) {
    stations_[idx].status = "关闭";
    ++cnt;
  }
  std::cout << "[换乘站关闭] 已关闭 \"" << name << "\" 的所有 "
            << cnt << " 个站点。\n";
  return cnt;
}

// ============================================================
//    运营管理扩展：线路停运 / 恢复
// ============================================================
int StationManager::closeLineStations(const std::string &line) {
  int cnt = 0;
  for (auto &st : stations_) {
    if (st.line == line && st.status == "开启") {
      st.status = "关闭";
      ++cnt;
    }
  }
  std::cout << "[线路停运] \"" << line << "\" 已停运，关闭 "
            << cnt << " 个站点。\n";
  return cnt;
}

int StationManager::openLineStations(const std::string &line) {
  int cnt = 0;
  for (auto &st : stations_) {
    if (st.line == line && st.status == "关闭") {
      st.status = "开启";
      ++cnt;
    }
  }
  std::cout << "[线路恢复] \"" << line << "\" 已恢复，开启 "
            << cnt << " 个站点。\n";
  return cnt;
}

// ============================================================
//    运营管理扩展：全网停运 / 恢复
// ============================================================
int StationManager::closeAllStations() {
  int cnt = 0;
  for (auto &st : stations_) {
    if (st.status == "开启") {
      st.status = "关闭";
      ++cnt;
    }
  }
  std::cout << "[全网停运] 已关闭全部 " << cnt << " 个站点。\n";
  return cnt;
}

int StationManager::openAllStations() {
  int cnt = 0;
  for (auto &st : stations_) {
    if (st.status == "关闭") {
      st.status = "开启";
      ++cnt;
    }
  }
  std::cout << "[全网恢复] 已开启全部 " << cnt << " 个站点。\n";
  return cnt;
}
