//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ctime>
#include <iostream>
#include <string>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/tokenizer.hpp>

#define ROW 3
#define COL 3

using boost::asio::ip::udp;
int queues_table[ROW][COL] = {0};
int processed_table[ROW][COL] = {0};


void printMatrix(int mat[ROW][COL])
{
    std::cout<<"\n Printing Matrix : \n";
    for(int i=0 ; i<=ROW-1 ; i++)
    {
        for(int j=0 ; j<=COL-1 ; j++)
            std::cout<< *(*(mat+i)+j)<<" ";
        std::cout<<std::endl;
    }
    std::cout<<std::endl;
}

std::string make_daytime_string()
{
  using namespace std; // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

class udp_server
{
public:
  udp_server(boost::asio::io_service& io_service, int port)
    : socket_(io_service, udp::endpoint(udp::v4(), port))
  {
    start_receive();
  }

private:
  void start_receive()
  {
    socket_.async_receive_from(
        boost::asio::buffer(recv_buffer_), remote_endpoint_,
        boost::bind(&udp_server::handle_receive, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

  void handle_receive(const boost::system::error_code& error,
      std::size_t bytes_transferred)
  {
    std::string msg = std::string(reinterpret_cast<const char*>(recv_buffer_.data()), static_cast<int>(bytes_transferred));
    std::cout << msg << std::endl;

    // Extract the service name
    boost::tokenizer<boost::char_separator<char>> tok(msg, boost::char_separator<char>(":"));

    std::vector<std::string> tokens;
    std::copy(tok.begin(), tok.end(), std::back_inserter(tokens));

    // A valid message should consist of two tokens: the service
    // name and the name of the method to call.

    if (tokens.size() == 4) {
      int processed_msgs = std::stoi(tokens[0]);
      std::vector<std::string> proc_service_tokens, sender_service_tokens;

      // Extract processor-service index
      boost::tokenizer<boost::char_separator<char>> proc_index_tok(tokens[2], boost::char_separator<char>("-"));
      std::copy(proc_index_tok.begin(), proc_index_tok.end(), std::back_inserter(proc_service_tokens));
      int proc_index = std::stoi(proc_service_tokens[1])-1;
      // Extract sender-service index
      boost::tokenizer<boost::char_separator<char>>  sender_index_tok(tokens[3], boost::char_separator<char>("-"));
      std::copy(sender_index_tok.begin(), sender_index_tok.end(), std::back_inserter(sender_service_tokens));
      int sender_index = std::stoi(sender_service_tokens[1])-1;

      // update queue entry
      mtx_.lock();
      queues_table[sender_index][proc_index] = queues_table[sender_index][proc_index]  
                                                - std::max(0, processed_msgs - (int)processed_table[proc_index][sender_index]);
      
      // update processed table
      processed_table[proc_index][sender_index] = std::max((int)processed_table[proc_index][sender_index], processed_msgs);
      mtx_.unlock();

      mtx_.lock();
      std::cout << "Q" << std::endl;
      printMatrix(queues_table);
      std::cout << "P" << std::endl;
      printMatrix(processed_table);
      mtx_.unlock();

    } else if (tokens.size() == 3) {
      std::vector<std::string> proc_service_tokens, sender_service_tokens;

      // Extract processor-service index
      boost::tokenizer<boost::char_separator<char>> proc_index_tok(tokens[2], boost::char_separator<char>("-"));
      std::copy(proc_index_tok.begin(), proc_index_tok.end(), std::back_inserter(proc_service_tokens));
      int proc_index = std::stoi(proc_service_tokens[1])-1;
      // Extract sender-service index
      boost::tokenizer<boost::char_separator<char>>  sender_index_tok(tokens[1], boost::char_separator<char>("-"));
      std::copy(sender_index_tok.begin(), sender_index_tok.end(), std::back_inserter(sender_service_tokens));
      int sender_index = std::stoi(sender_service_tokens[1])-1;

      // update queue entry
      mtx_.lock();
      queues_table[sender_index][proc_index]++;
      mtx_.unlock();
    }
    
    if (!error || error == boost::asio::error::message_size)
    {

      boost::shared_ptr<std::string> message(
          new std::string(make_daytime_string()));

      socket_.async_send_to(boost::asio::buffer(*message), remote_endpoint_,
          boost::bind(&udp_server::handle_send, this, message,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));

      start_receive();
    }
  }

  void handle_send(boost::shared_ptr<std::string> /*message*/,
      const boost::system::error_code& /*error*/,
      std::size_t /*bytes_transferred*/)
  {
  }

  udp::socket socket_;
  udp::endpoint remote_endpoint_;
  boost::array<char, 1024> recv_buffer_;
  boost::mutex mtx_;
};

void add_thread() {
  try
  {
    boost::asio::io_service io_service;
    udp_server server(io_service, 1337);
    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
}

void remove_thread() {
  try
  {
    boost::asio::io_service io_service;
    udp_server server(io_service, 1336);
    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
}

int main()
{
  boost::thread add_t{add_thread}, rm_t{remove_thread};
  add_t.join();
  rm_t.join();
  
  return 0;
}
