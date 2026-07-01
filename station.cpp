#include "station.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

// 重建站点索引（按 id 和 name）
namespace {
void rebuildIndexes(
    const std::vector<Station> &stations,
    std::unordered_map<int, size_t> &indexById,
    std::unordered_map<std::string, std::vector<size_t>> &indexesByName) {
  indexById.clear();
  indexesByName.clear();
  for (size_t i = 0; i < stations.size(); ++i) {
    indexById[stations[i].id] = i;
    indexesByName[stations[i].name].push_back(i);
  }
}
} // namespace

std::string StationManager::trim(const std::string &s) {
  size_t b = 0, e = s.size();
  if (e >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB &&
      (unsigned char)s[2] == 0xBF) {
    b = 3;
  }
  auto isSpace = [](unsigned char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
  };
  while (b < e && isSpace((unsigned char)s[b]))
    ++b;
  while (e > b && isSpace((unsigned char)s[e - 1]))
    --e;
  return s.substr(b, e - b);
}

// 将 UTF-8 路径转为系统原生编码（Windows: UTF-8→UTF-16→ANSI）
std::string StationManager::toNativePath(const std::string &utf8Path) {
#ifdef _WIN32
  if (utf8Path.empty())
    return utf8Path;
  int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8Path.c_str(), -1, nullptr, 0);
  if (wlen <= 0)
    return utf8Path;
  std::wstring wide(wlen, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8Path.c_str(), -1, &wide[0], wlen);
  int alen = WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, nullptr, 0,
                                 nullptr, nullptr);
  if (alen <= 0)
    return utf8Path;
  std::string ansi(alen, '\0');
  WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, &ansi[0], alen, nullptr,
                      nullptr);
  // 去掉末尾 '\0'
  if (!ansi.empty() && ansi.back() == '\0')
    ansi.pop_back();
  return ansi;
#else
  return utf8Path;
#endif
}

// 解析一行 CSV，处理引号与逗号
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

// 从 CSV 加载站点数据
bool StationManager::loadFromCSV(const std::string &csvPath) {
  std::ifstream fin(toNativePath(csvPath));
  if (!fin.is_open()) {
    std::cerr << "[Station] 无法打开文件: " << csvPath << std::endl;
    return false;
  }
  std::string line;
  bool first = true;
  stations_.clear();
  while (std::getline(fin, line)) {
    line = trim(line);
    if (line.empty())
      continue;
    auto fields = parseCSVLine(line);
    if (first) {
      first = false;
      if (fields.size() >= 1 && fields[0] == "id")
        continue;
    }
    if (fields.size() != 4) {
      std::cerr << "[Station] 站点数据字段数量错误: " << line << std::endl;
      continue;
    }
    Station st;
    try {
      st.id = std::stoi(fields[0]);
    } catch (...) {
      std::cerr << "[Station] 非法站点 id: " << fields[0] << std::endl;
      continue;
    }
    st.name = fields[1];
    st.line = fields[2];
    st.status = fields[3];
    if (st.name.empty() || st.line.empty() ||
        (st.status != "开启" && st.status != "关闭")) {
      std::cerr << "[Station] 非法站点记录: " << line << std::endl;
      continue;
    }
    stations_.push_back(st);
  }
  fin.close();
  rebuildIndexes(stations_, indexById_, indexesByName_);
  return true;
}

// 加载初始状态快照用于一键恢复
bool StationManager::loadInitFromCSV(const std::string &csvPath) {
  std::ifstream fin(toNativePath(csvPath));
  if (!fin.is_open()) {
    std::cerr << "[Station] 无法打开初始化文件: " << csvPath << std::endl;
    return false;
  }
  std::string line;
  bool first = true;
  initialSnapshot_.clear();
  while (std::getline(fin, line)) {
    line = trim(line);
    if (line.empty())
      continue;
    auto fields = parseCSVLine(line);
    if (first) {
      first = false;
      if (fields.size() >= 1 && fields[0] == "id")
        continue;
    }
    if (fields.size() != 4) {
      std::cerr << "[Station] 初始化数据字段数量错误: " << line << std::endl;
      continue;
    }
    Station st;
    try {
      st.id = std::stoi(fields[0]);
    } catch (...) {
      std::cerr << "[Station] 初始化数据 id 非法: " << fields[0] << std::endl;
      continue;
    }
    st.name = fields[1];
    st.line = fields[2];
    st.status = fields[3];
    initialSnapshot_.push_back(st);
  }
  fin.close();
  return true;
}

// 批量更新站点状态，返回成功更新的数量
int StationManager::batchUpdateFromCSV(const std::string &csvPath) {
  // 路径预检
  std::ifstream precheck(toNativePath(csvPath));
  if (!precheck.is_open()) {
    std::cerr << "[Station] 文件不存在或无法访问: " << csvPath << std::endl;
    return -1;
  }
  if (precheck.peek() == std::ifstream::traits_type::eof()) {
    std::cerr << "[Station] 路径是目录或文件为空: " << csvPath << std::endl;
    return -1;
  }
  precheck.close();

  std::ifstream fin(toNativePath(csvPath));
  if (!fin.is_open()) {
    std::cerr << "[Station] 无法打开批量更新文件: " << csvPath << std::endl;
    return -1;
  }

  struct PendingUpdate {
    std::string name;
    std::string line;
    std::string status;
  };

  std::string line;
  bool first = true;
  int recordCount = 0;
  int success = 0, fail = 0;
  std::vector<PendingUpdate> pending;
  while (std::getline(fin, line)) {
    line = trim(line);
    if (line.empty())
      continue;
    auto fields = parseCSVLine(line);
    if (first) {
      first = false;
      if (!fields.empty() &&
          (fields[0] == "name" || fields[0] == "id" ||
           fields[0] == "站点名称" || fields[0] == "站点ID")) {
        continue;
      }
    }
    if (fields.size() == 2) {
      ++recordCount;
      int id = -1;
      try {
        id = std::stoi(fields[0]);
      } catch (...) {
        std::cout << "更新文件格式错误，终止更新。\n";
        return -1;
      }
      const Station *s = findById(id);
      if (!s) {
        std::cout << "未匹配到对应站点: " << fields[0] << "\n";
        ++fail;
        continue;
      }
      if (fields[1] != "开启" && fields[1] != "关闭") {
        std::cout << "非法状态值: " << fields[1] << "，必须为“开启/关闭”。\n";
        ++fail;
        continue;
      }
      pending.push_back({s->name, s->line, fields[1]});
      continue;
    }
    if (fields.size() == 3) {
      ++recordCount;
      std::string name = fields[0];
      std::string ln = fields[1];
      std::string st = fields[2];
      if (st != "开启" && st != "关闭") {
        std::cout << "非法状态值: " << st << "，必须为“开启/关闭”。\n";
        ++fail;
        continue;
      }
      pending.push_back({name, ln, st});
      continue;
    }
    std::cout << "更新文件格式错误，终止更新。\n";
    return -1;
  }
  fin.close();

  if (recordCount == 0) {
    std::cout << "未检测到有效更新记录。\n";
    return 0;
  }

  for (const auto &upd : pending) {
    if (setStationStatus(upd.name, upd.line, upd.status))
      ++success;
    else {
      std::cout << "未匹配到对应站点: " << upd.name << " (" << upd.line
                << ")\n";
      ++fail;
    }
  }

  std::cout << "[批量更新] 成功 " << success << " 条，失败 " << fail
            << " 条。\n";
  return success;
}

// 模糊搜索站点（不区分大小写）
std::vector<Station>
StationManager::fuzzySearch(const std::string &keyword) const {
  std::vector<Station> res;
  if (keyword.empty())
    return res;
  std::string loweredKeyword;
  loweredKeyword.reserve(keyword.size());
  for (unsigned char ch : keyword)
    loweredKeyword.push_back((char)std::tolower(ch));
  for (const auto &st : stations_) {
    std::string loweredName;
    loweredName.reserve(st.name.size());
    for (unsigned char ch : st.name)
      loweredName.push_back((char)std::tolower(ch));
    if (loweredName.find(loweredKeyword) != std::string::npos) {
      res.push_back(st);
    }
  }
  return res;
}

// 站点查询：按名称 / id / 关闭状态 / 线路
std::vector<Station>
StationManager::getStationsByName(const std::string &name) const {
  std::vector<Station> res;
  auto it = indexesByName_.find(name);
  if (it != indexesByName_.end()) {
    for (size_t i : it->second)
      res.push_back(stations_[i]);
  }
  return res;
}

const Station *StationManager::findById(int id) const {
  auto it = indexById_.find(id);
  if (it == indexById_.end())
    return nullptr;
  return &stations_[it->second];
}

std::vector<Station> StationManager::getClosedStations() const {
  std::vector<Station> res;
  for (const auto &st : stations_) {
    if (st.status == "关闭")
      res.push_back(st);
  }
  return res;
}

std::vector<Station>
StationManager::getStationsByLine(const std::string &line) const {
  std::vector<Station> res;
  for (const auto &st : stations_) {
    if (st.line == line)
      res.push_back(st);
  }
  return res;
}

// 获取所有线路名，按编号排序
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

// 站点状态更新与恢复（含静默恢复）
bool StationManager::setStationStatus(const std::string &name,
                                      const std::string &line,
                                      const std::string &newStatus) {
  auto it = indexesByName_.find(name);
  if (it == indexesByName_.end())
    return false;
  for (size_t i : it->second) {
    if (stations_[i].line == line) {
      stations_[i].status = newStatus;
      return true;
    }
  }
  return false;
}

void StationManager::restoreInitialStatus() {
  if (restoreInitialStatusSilent()) {
    std::cout << "[恢复] 已将所有站点状态恢复为初始快照。\n";
  }
}

bool StationManager::restoreInitialStatusSilent() {
  if (initialSnapshot_.empty()) {
    return false;
  }
  for (const auto &initial : initialSnapshot_) {
    auto it = indexById_.find(initial.id);
    if (it != indexById_.end()) {
      stations_[it->second].status = initial.status;
    }
  }
  return true;
}

// 保存当前站点状态到 CSV（写入 UTF-8 BOM 防乱码）
bool StationManager::saveCurrentToCSV(const std::string &csvPath) const {
  std::ofstream fout(toNativePath(csvPath), std::ios::binary);
  if (!fout.is_open()) {
    std::cerr << "[Station] 无法写入文件: " << csvPath << std::endl;
    return false;
  }
  const unsigned char bom[3] = {0xEF, 0xBB, 0xBF};
  fout.write(reinterpret_cast<const char *>(bom), 3);
  fout << "id,name,line,status\n";
  for (const auto &st : stations_) {
    fout << st.id << "," << st.name << "," << st.line << "," << st.status
         << "\n";
  }
  fout.close();
  return true;
}

// 状态判断与格式化显示（关闭站点表 / 线路站点表）
bool StationManager::isClosed(int id) const {
  auto it = indexById_.find(id);
  if (it == indexById_.end())
    return true;
  return stations_[it->second].status == "关闭";
}

void StationManager::showClosedStations(std::ostream &output) const {
  auto closed = getClosedStations();
  output << "  ==== 当前关闭的站点 ====\n";
  if (closed.empty()) {
    output << "  （无）\n";
    return;
  }
  output << "  " << std::left << std::setw(8) << "ID" << std::setw(20) << "站名"
         << std::setw(10) << "线路" << std::setw(8) << "状态" << "\n";
  output << "  " << std::string(46, '-') << "\n";
  for (const auto &station : closed) {
    output << "  " << std::left << std::setw(8) << station.id << std::setw(20)
           << station.name << std::setw(10) << station.line << std::setw(8)
           << station.status << "\n";
  }
  output << "  共 " << closed.size() << " 个关闭站点。\n";
}

void StationManager::showStationsByLine(const std::string &line,
                                        std::ostream &output) const {
  auto stations = getStationsByLine(line);
  output << "  ==== 线路 [" << line << "] 的所有站点 ====\n";
  if (stations.empty()) {
    output << "  （无）\n";
    return;
  }
  output << "  " << std::left << std::setw(8) << "ID" << std::setw(20) << "站名"
         << std::setw(10) << "线路" << std::setw(8) << "状态" << "\n";
  output << "  " << std::string(46, '-') << "\n";
  for (const auto &station : stations) {
    output << "  " << std::left << std::setw(8) << station.id << std::setw(20)
           << station.name << std::setw(10) << station.line << std::setw(8)
           << station.status << "\n";
  }
  output << "  共 " << stations.size() << " 个站点。\n";
}

int StationManager::nextStationId() const {
  int maxId = 0;
  for (const auto &station : stations_) {
    if (station.id > maxId)
      maxId = station.id;
  }
  return maxId + 1;
}

// 添加新站点，返回新 id（同名同线已存在返回 -1）
int StationManager::addStation(const std::string &name, const std::string &line,
                               const std::string &status) {
  auto sameNameStations = getStationsByName(name);
  for (const auto &station : sameNameStations) {
    if (station.line == line)
      return -1;
  }

  Station station;
  station.id = nextStationId();
  station.name = name;
  station.line = line;
  station.status = status;
  stations_.push_back(station);
  rebuildIndexes(stations_, indexById_, indexesByName_);
  return station.id;
}

// 站点增删与换乘判断
bool StationManager::removeStation(const std::string &name,
                                   const std::string &line) {
  auto it = indexesByName_.find(name);
  if (it == indexesByName_.end())
    return false;

  std::vector<size_t> indexesToErase;
  for (size_t index : it->second) {
    if (stations_[index].line == line)
      indexesToErase.push_back(index);
  }
  if (indexesToErase.empty())
    return false;

  std::sort(indexesToErase.rbegin(), indexesToErase.rend());
  for (size_t index : indexesToErase)
    stations_.erase(stations_.begin() + index);

  rebuildIndexes(stations_, indexById_, indexesByName_);
  return true;
}

bool StationManager::isTransferStation(const std::string &name) const {
  auto it = indexesByName_.find(name);
  if (it == indexesByName_.end())
    return false;
  return it->second.size() > 1;
}

// 关闭换乘站所有同名站点
int StationManager::closeTransferStation(const std::string &name) {
  auto it = indexesByName_.find(name);
  if (it == indexesByName_.end()) {
    std::cout << "[换乘站关闭] 未找到站点 \"" << name << "\"。\n";
    return 0;
  }
  if (it->second.size() <= 1) {
    std::cout << "[换乘站关闭] \"" << name
              << "\" 不是换乘站，仅有 1 条线路。\n";
    return 0;
  }
  int cnt = 0;
  for (size_t idx : it->second) {
    stations_[idx].status = "关闭";
    ++cnt;
  }
  std::cout << "[换乘站关闭] 已关闭 \"" << name << "\" 的所有 " << cnt
            << " 个站点。\n";
  return cnt;
}

// 线路 / 全网停运与恢复（带中文打印，供 CLI 使用）
int StationManager::closeLineStations(const std::string &line) {
  int cnt = 0;
  for (auto &st : stations_) {
    if (st.line == line && st.status == "开启") {
      st.status = "关闭";
      ++cnt;
    }
  }
  std::cout << "[线路停运] \"" << line << "\" 已停运，关闭 " << cnt
            << " 个站点。\n";
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
  std::cout << "[线路恢复] \"" << line << "\" 已恢复，开启 " << cnt
            << " 个站点。\n";
  return cnt;
}

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

// 静默版本：不打印中文提示，供 Web 子进程调用
int StationManager::closeTransferStationSilent(const std::string &name) {
  auto it = indexesByName_.find(name);
  if (it == indexesByName_.end())
    return 0;
  if (it->second.size() <= 1)
    return 0;
  int cnt = 0;
  for (size_t idx : it->second) {
    stations_[idx].status = "关闭";
    ++cnt;
  }
  return cnt;
}

int StationManager::closeLineStationsSilent(const std::string &line) {
  int cnt = 0;
  for (auto &st : stations_) {
    if (st.line == line && st.status == "开启") {
      st.status = "关闭";
      ++cnt;
    }
  }
  return cnt;
}

int StationManager::openLineStationsSilent(const std::string &line) {
  int cnt = 0;
  for (auto &st : stations_) {
    if (st.line == line && st.status == "关闭") {
      st.status = "开启";
      ++cnt;
    }
  }
  return cnt;
}

int StationManager::closeAllStationsSilent() {
  int cnt = 0;
  for (auto &st : stations_) {
    if (st.status == "开启") {
      st.status = "关闭";
      ++cnt;
    }
  }
  return cnt;
}

int StationManager::openAllStationsSilent() {
  int cnt = 0;
  for (auto &st : stations_) {
    if (st.status == "关闭") {
      st.status = "开启";
      ++cnt;
    }
  }
  return cnt;
}
