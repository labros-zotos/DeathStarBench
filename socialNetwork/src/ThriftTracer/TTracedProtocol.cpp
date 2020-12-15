#include "TTracedProtocol.h"
#include <thrift/protocol/TProtocolDecorator.h>
#include <iostream>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>

#include "UDPClient.h"

namespace social_network{
using json = nlohmann::json;

int load_config_file_(const std::string &file_name, json *config_json) {
  std::ifstream json_file;
  json_file.open(file_name);
  if (json_file.is_open()) {
    json_file >> *config_json;
    json_file.close();
    return 0;
  }
  else {
    // LOG(error) << "Cannot open service-config.json"; TODO: Log error
    return -1;
  }
};

} //namespace social_network

using namespace social_network;

namespace apache {
namespace thrift {
namespace protocol {
  uint32_t TTracedProtocol::writeMessageBegin_virt(const std::string& _name,
                                                        const TMessageType _type,
                                                        const int32_t _seqid) {
    
    // Log message to supervisor
    std::cout << "Sending message from " << service_id << " to " << receiver_addr << std::endl;
    boost::asio::io_service io_service;

    json config_json;
    if (load_config_file_("config/service-config.json", &config_json) != 0) {
      exit(EXIT_FAILURE);
    }

    std::string supervisor_addr = config_json["supervisor-service"]["addr"];
    int supervisor_port = config_json["supervisor-service"]["port"];
    
    UDPClient client(io_service, supervisor_addr, supervisor_port);

    client.send("add:"+service_id+":"+receiver_addr);

    if (_type == T_CALL || _type == T_ONEWAY) {
      return TProtocolDecorator::writeMessageBegin_virt(service_id + separator + _name,
                                                        _type,
                                                        _seqid);
    } else {
      return TProtocolDecorator::writeMessageBegin_virt(_name, _type, _seqid);
    }
  }

  uint32_t TTracedProtocol::readMessageEnd_virt() {
    printf("Receiving a response");
    return TProtocolDecorator::readMessageEnd_virt();
}
}
}
}