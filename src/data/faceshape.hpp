#pragma once

#include <array>
#include <cmath>
#include <locale>
#include <map>
#include <sstream>
#include <string>

class FaceShape {
public:
  using Point = std::array<float, 3>;

  void Load(const std::string& data) {
    offsets.clear();
    model.clear();
    std::istringstream input(data);
    input.imbue(std::locale::classic());
    std::string header;
    if (!std::getline(input, header))
      return;
    if (header.rfind("face-v2 ", 0) == 0) {
      std::istringstream fields(header.substr(8));
      std::string digest;
      fields >> digest;
      if (digest.size() == 64 && digest.find_first_not_of("0123456789abcdef") == std::string::npos)
        model = "media/objects/players/faces/" + digest + ".ase";
      return;
    }
    if (header.rfind("face-v1 ", 0) != 0)
      return;
    std::map<Point, Point> parsed;
    std::string line;
    while (std::getline(input, line)) {
      std::istringstream row(line);
      row.imbue(std::locale::classic());
      Point source, delta;
      if (!(row >> source[0] >> source[1] >> source[2] >> delta[0] >> delta[1] >> delta[2]))
        return;
      row >> std::ws;
      if (!row.eof() || parsed.size() >= 4096)
        return;
      for (int i = 0; i < 3; ++i) {
        if (!std::isfinite(source[i]) || !std::isfinite(delta[i]) || std::abs(delta[i]) > 0.1f)
          return;
      }
      parsed[source] = delta;
    }
    offsets.swap(parsed);
  }

  const std::string& Model() const { return model; }

  void Apply(float* position) const {
    if (offsets.empty())
      return;
    auto it = offsets.find({position[0], position[1], position[2]});
    if (it != offsets.end()) {
      for (int i = 0; i < 3; ++i)
        position[i] += it->second[i];
    }
  }

private:
  std::string model;
  std::map<Point, Point> offsets;
};
