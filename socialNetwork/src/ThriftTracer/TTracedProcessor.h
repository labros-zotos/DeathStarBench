#ifndef THRIFT_TTRACEDPROCESSOR_H_
#define THRIFT_TTRACEDPROCESSOR_H_ 1

#include <thrift/protocol/TProtocolDecorator.h>
#include <thrift/TApplicationException.h>
#include <thrift/TProcessor.h>
#include <boost/tokenizer.hpp>
#include <iostream>
#include <string>
#include "../utils.h"
#include "UDPClient.h"

json config_json;
if (load_config_file("config/service-config.json", &config_json) != 0) {
  exit(EXIT_FAILURE);
}

std::string secret = config_json["secret"];

std::string supervisor_addr = config_json["supervisor-service"]["addr"];
int supervisor_port = config_json["supervisor-service"]["port"];

namespace apache {
namespace thrift {
namespace protocol {

/**
 *  To be able to work with any protocol, we needed
 *  to allow them to call readMessageBegin() and get a TMessage in exactly
 *  the standard format, without the service name prepended to TMessage.name.
 */
class StoredMessageProtocol : public TProtocolDecorator {
public:
  StoredMessageProtocol(std::shared_ptr<protocol::TProtocol> _protocol,
                        const std::string& _name,
                        const TMessageType _type,
                        const int32_t _seqid)
    : TProtocolDecorator(_protocol), name(_name), type(_type), seqid(_seqid) {}

  uint32_t readMessageBegin_virt(std::string& _name, TMessageType& _type, int32_t& _seqid) override {

    _name = name;
    _type = type;
    _seqid = seqid;

    return 0; // (Normal TProtocol read functions return number of bytes read)
  }

  std::string name;
  TMessageType type;
  int32_t seqid;
};
} // namespace protocol

/**
 * <code>TTracedProcessor</code> is a <code>TProcessor</code> allowing
 * a single <code>TServer</code> to provide multiple services.
 *
 * <p>To do so, you instantiate the processor and then register additional
 * processors with it, as shown in the following example:</p>
 *
 * <blockquote><code>
 *     std::shared_ptr<TTracedProcessor> processor(new TTracedProcessor());
 *
 *     processor->registerProcessor(
 *         "Calculator",
 *         std::shared_ptr<TProcessor>( new CalculatorProcessor(
 *             std::shared_ptr<CalculatorHandler>( new CalculatorHandler()))));
 *
 *     processor->registerProcessor(
 *         "WeatherReport",
 *         std::shared_ptr<TProcessor>( new WeatherReportProcessor(
 *             std::shared_ptr<WeatherReportHandler>( new WeatherReportHandler()))));
 *
 *     std::shared_ptr<TServerTransport> transport(new TServerSocket(9090));
 *     TSimpleServer server(processor, transport);
 *
 *     server.serve();
 * </code></blockquote>
 */
class TTracedProcessor : public TProcessor {
public:
  typedef std::map<std::string, std::shared_ptr<TProcessor> > services_t;

  /**
    * 'Register' a service with this <code>TTracedProcessor</code>.  This
    * allows us to broker requests to individual services by using the service
    * name to select them at request time.
    *
    * \param [in] service_id  ID of a service, has to be identical to the name
    *                         declared in the Thrift IDL, e.g. "WeatherReport".
    * \param [in] processor   Implementation of a service, usually referred to
    *                         as "handlers", e.g. WeatherReportHandler,
    *                         implementing WeatherReportIf interface.
    */
  void registerProcessor(const std::string& _service_id, std::shared_ptr<TProcessor> _processor) {
    service_id = _service_id;
    processor = _processor;
  }

  /**
   * Chew up invalid input and return an exception to throw.
   */
  TException protocol_error(std::shared_ptr<protocol::TProtocol> in,
                            std::shared_ptr<protocol::TProtocol> out,
                            const std::string& name, 
                            int32_t seqid, 
                            const std::string& msg) const {
    in->skip(::apache::thrift::protocol::T_STRUCT);
    in->readMessageEnd();
    in->getTransport()->readEnd();
    ::apache::thrift::TApplicationException
      x(::apache::thrift::TApplicationException::PROTOCOL_ERROR,
        "TTracedProcessor: " + msg);
    out->writeMessageBegin(name, ::apache::thrift::protocol::T_EXCEPTION, seqid);
    x.write(out.get());
    out->writeMessageEnd();
    out->getTransport()->writeEnd();
    out->getTransport()->flush();
    return TException(msg);
}
   
  /**
   * This implementation of <code>process</code> performs the following steps:
   *
   * <ol>
   *     <li>Read the beginning of the message.</li>
   *     <li>Extract the service name from the message.</li>
   *     <li>Using the service name to locate the appropriate processor.</li>
   *     <li>Dispatch to the processor, with a decorated instance of TProtocol
   *         that allows readMessageBegin() to return the original TMessage.</li>
   * </ol>
   *
   * \throws TException If the message type is not T_CALL or T_ONEWAY, if
   * the service name was not found in the message, or if the service
   * name was not found in the service map.
   */
  bool process(std::shared_ptr<protocol::TProtocol> in,
               std::shared_ptr<protocol::TProtocol> out,
               void* connectionContext) override {
    std::string name;
    protocol::TMessageType type;
    int32_t seqid;

    // Use the actual underlying protocol (e.g. TBinaryProtocol) to read the
    // message header.  This pulls the message "off the wire", which we'll
    // deal with at the end of this method.
    in->readMessageBegin(name, type, seqid);

    if (type != protocol::T_CALL && type != protocol::T_ONEWAY) {
      // Unexpected message type.
      throw protocol_error(in, out, name, seqid, "Unexpected message type");
    }

    // Extract the service name
    boost::tokenizer<boost::char_separator<char> > tok(name, boost::char_separator<char>(":"));

    std::vector<std::string> tokens;
    std::copy(tok.begin(), tok.end(), std::back_inserter(tokens));

    // A valid message should consist of two tokens: the service
    // name and the name of the method to call.

    if (tokens.size() == 2) {
        // Increase the number of processed requests for the sender-microservice
        rpc_processed[0]++; 

        // Acknowledge message processing to supervisor
        std::cout << "Receiving message from " << tokens[0] << std::endl;

        boost::asio::io_service io_service;
        UDPClient client(io_service, supervisor_addr, supervisor_port-1);

        client.send(std::to_string(rpc_processed[0])+":proc:"+service_id+":"+tokens[0]);

        // Let the processor registered for this service name
        // process the message.
        return processor
            ->process(std::shared_ptr<protocol::TProtocol>(
                          new protocol::StoredMessageProtocol(in, tokens[1], type, seqid)),
                      out,
                      connectionContext);
    } else {
		throw protocol_error(in, out, name, seqid,
		    "Wrong number of tokens.");
    }
  }

private:
  std::string service_id;
  std::shared_ptr<TProcessor> processor;
  // Keep count of how many requests were proccessed from each microservice.
  int rpc_processed[2] = {0};
};
}
}

#endif // THRIFT_TTRACEDPROCESSOR_H_