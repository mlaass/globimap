#pragma once

typedef std::vector<std::vector<globimap::LayerConfig>> config_t;

void get_configurations(config_t &cfgs,
                        const std::vector<uint> &sizes = {16, 24, 32}) {

  std::vector<uint> bits{1, 8, 16, 32, 64};
  std::vector<uint> sz = sizes;

  std::vector<std::vector<uint>> sz_perms;
  std::cout << "make permutations ..." << std::endl;
  do {
    sz_perms.push_back(sz);
  } while (std::next_permutation(sz.begin(), sz.end()));

  std::cout << "make configurations ..." << sz_perms.size() << std::endl;

  auto si = 0;
  for (auto &s : sz_perms) {
    std::cout << si << " : " << s[0] << ", ";
    for (auto a = 0; a < 2; a++) {
      std::vector<globimap::LayerConfig> x = {};
      std::cout << a << ", ";
      x.push_back({bits[a], s[0]});
      cfgs.push_back(x);
      for (auto b = a + 1; b < 5; b++) {
        std::cout << b << ", ";
        x.push_back({bits[b], s[1]});
        cfgs.push_back(x);
        for (auto c = b + 1; c < 5; c++) {
          std::cout << c << ", ";
          x.push_back({bits[c], s[2]});
          cfgs.push_back(x);
          x.pop_back();
        }
        x.pop_back();
      }
    }

    std::cout << "\n" << std::endl;
    si++;
  }
}

std::string configs_to_string(const config_t &cfgs) {
  std::cout << "write configurations ..." << std::endl;
  std::stringstream ss;

  ss << "{ \"config\":[" << std ::endl;
  int i = 0;
  for (auto &cfg : cfgs) {
    // ss << "\"cfg" << i << "\": ";
    ss << "[\n";
    auto l = 0;
    for (auto c : cfg) {
      ss << "{\"bits\": " << c.bits << ", \"logsize\": " << c.logsize << "}"
         << ((l == cfg.size() - 1) ? "" : ",\n");
      l++;
    }
    ss << "]" << ((i == cfgs.size() - 1) ? "" : ",\n");
    i++;
  }
  ss << "\n]}" << std ::endl;

  return ss.str();
}
