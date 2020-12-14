#ifndef THRIFT_TTRACEDPROTOCOL_H_
#define THRIFT_TTRACEDPROTOCOL_H_ 1

#include <thrift/protocol/TProtocolDecorator.h>

namespace apache {
namespace thrift {
namespace protocol {
using std::shared_ptr;

/**
 * <code>TTracedProtocol</code> is a protocol-independent concrete decorator
 * that allows a Thrift client to communicate with a multiplexing Thrift server,
 * by prepending the service name to the function name during function calls.
 *
 * \note THIS IS NOT USED BY SERVERS.  On the server, use
 * {@link apache::thrift::TMultiplexedProcessor TMultiplexedProcessor} to handle requests
 * from a multiplexing client.
 *
 * This example uses a single socket transport to invoke two services:
 *
 * <blockquote><code>
 *     shared_ptr<TSocket> transport(new TSocket("localhost", 9090));
 *     transport->open();
 *
 *     shared_ptr<TBinaryProtocol> protocol(new TBinaryProtocol(transport));
 *
 *     shared_ptr<TTracedProtocol> mp1(new TTracedProtocol(protocol, "Calculator"));
 *     shared_ptr<CalculatorClient> service1(new CalculatorClient(mp1));
 *
 *     shared_ptr<TTracedProtocol> mp2(new TTracedProtocol(protocol, "WeatherReport"));
 *     shared_ptr<WeatherReportClient> service2(new WeatherReportClient(mp2));
 *
 *     service1->add(2,2);
 *     int temp = service2->getTemperature();
 * </code></blockquote>
 *
 * @see apache::thrift::protocol::TProtocolDecorator
 */
class TTracedProtocol : public TProtocolDecorator {
public:
  /**
   * Wrap the specified protocol, allowing it to be used to communicate with a
   * multiplexing server.  The <code>serviceName</code> is required as it is
   * prepended to the message header so that the multiplexing server can broker
   * the function call to the proper service.
   *
   * \param _protocol    Your communication protocol of choice, e.g. <code>TBinaryProtocol</code>.
   * \param _service_id The service id of the service communicating via this protocol.
   * \param _receiver_addr The ip address of the service the message is sent to.
   */
  TTracedProtocol(shared_ptr<TProtocol> _protocol, const std::string& _service_id, const std::string& _receiver_addr)
    : TProtocolDecorator(_protocol), service_id(_service_id), receiver_addr(_receiver_addr), separator(":") {}
  ~TTracedProtocol() override = default;

  /**
   * Prepends the service name to the function name, separated by TMultiplexedProtocol::SEPARATOR.
   *
   * \param [in] _name   The name of the method to be called in the service.
   * \param [in] _type   The type of message
   * \param [in] _name   The sequential id of the message
   *
   * \throws TException  Passed through from wrapped <code>TProtocol</code> instance.
   */
  uint32_t writeMessageBegin_virt(const std::string& _name,
                                  const TMessageType _type,
                                  const int32_t _seqid) override;
  uint32_t readMessageEnd_virt() override;

private:
  const std::string service_id;
  const std::string receiver_addr;
  const std::string separator;
};
}
}
}

#endif // THRIFT_TTRACEDPROTOCOL_H_