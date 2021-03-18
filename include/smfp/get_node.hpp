#ifndef SMFP_GET_NODE_HPP
#define SMFP_GET_NODE_HPP
#include <string>
#include <nlohmann/json.hpp>

namespace smfp {
  nlohmann::json get_node(
    const nlohmann::json &config,
    const std::string &name
  );
}

#endif

