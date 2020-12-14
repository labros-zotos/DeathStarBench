#ifndef SOCIAL_NETWORK_MICROSERVICES_TRACEDTHRIFTCLIENT_H
#define SOCIAL_NETWORK_MICROSERVICES_TRACEDTHRIFTCLIENT_H

#include <string>
#include <thread>
#include <iostream>
#include <boost/log/trivial.hpp>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/stdcxx.h>
#include "logger.h"
#include "GenericClient.h"
#include "./ThriftTracer/TTracedProtocol.h"

namespace social_network {

using apache::thrift::protocol::TProtocol;
using apache::thrift::protocol::TBinaryProtocol;
using apache::thrift::transport::TFramedTransport;
using apache::thrift::transport::TSocket;
using apache::thrift::transport::TTransport;
using apache::thrift::TException;

template<class TTracedThriftClient>
class TracedThriftClient : public GenericClient {
 public:
  TracedThriftClient(const std::string &addr, int port, const std::string &service_id, const std::string &receiver_id);

  TracedThriftClient(const TracedThriftClient &) = delete;
  TracedThriftClient &operator=(const TracedThriftClient &) = delete;
  TracedThriftClient(TracedThriftClient<TTracedThriftClient> &&) = default;
  TracedThriftClient &operator=(TracedThriftClient &&) = default;

  ~TracedThriftClient() override;

  TTracedThriftClient *GetClient() const;

  void Connect() override;
  void Disconnect() override;
  void KeepAlive() override;
  void KeepAlive(int timeout_ms) override;
  bool IsConnected() override;

 private:
  TTracedThriftClient *_client;

  std::shared_ptr<TTransport> _socket;
  std::shared_ptr<TTransport> _transport;
  std::shared_ptr<TProtocol> _protocol;
};

template<class TTracedThriftClient>
TracedThriftClient<TTracedThriftClient>::TracedThriftClient(
    const std::string &addr, int port, const std::string &service_id, const std::string &receiver_id) {
  _addr = addr;
  _port = port;
  _socket = std::shared_ptr<TTransport>(new TSocket(addr, port));
  _transport = std::shared_ptr<TTransport>(new TFramedTransport(_socket));
  _protocol = std::shared_ptr<TTracedProtocol>(std::shared_ptr<TProtocol>(new TBinaryProtocol(_transport));, service_id, reciever_id);
  _client = new TTracedThriftClient(_protocol);
}

template<class TTracedThriftClient>
TracedThriftClient<TTracedThriftClient>::~TracedThriftClient() {
  Disconnect();
  delete _client;
}

template<class TTracedThriftClient>
TTracedThriftClient *TracedThriftClient<TTracedThriftClient>::GetClient() const {
  return _client;
}

template<class TTracedThriftClient>
bool TracedThriftClient<TTracedThriftClient>::IsConnected() {
  return _transport->isOpen();
}

template<class TTracedThriftClient>
void TracedThriftClient<TTracedThriftClient>::Connect() {
  if (!IsConnected()) {
    try {
      _transport->open();
    } catch (TException &tx) {
      throw tx;
    }
  }
}

template<class TTracedThriftClient>
void TracedThriftClient<TTracedThriftClient>::Disconnect() {
  if (IsConnected()) {
    try {
      _transport->close();
    } catch (TException &tx) {
      throw tx;
    }
  }
}

template<class TTracedThriftClient>
void TracedThriftClient<TTracedThriftClient>::KeepAlive() {

}

// TODO: Implement KeepAlive Timeout
template<class TTracedThriftClient>
void TracedThriftClient<TTracedThriftClient>::KeepAlive(int timeout_ms) {

}

} // namespace social_network


#endif //SOCIAL_NETWORK_MICROSERVICES_TRACEDTHRIFTCLIENT_H
