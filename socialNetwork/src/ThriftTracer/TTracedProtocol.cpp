#include "TTracedProtocol.h"
#include <thrift/protocol/TProtocolDecorator.h>
#include <iostream>
#include "udp_client.h"

namespace apache {
namespace thrift {
namespace protocol {
  uint32_t TTracedProtocol::writeMessageBegin_virt(const std::string& _name,
                                                        const TMessageType _type,
                                                        const int32_t _seqid) {
    
    // Log message to supervisor
    std::cout << "Sending message from " << service_id << " to " << receiver_addr << std::endl;
    boost::asio::io_service io_service;
    UDPClient client(io_service, "localhost", "1337");

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