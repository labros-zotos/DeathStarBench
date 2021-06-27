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
#include <pqxx/pqxx>
#include "../utils.h"

#define ROW 3
#define COL 3

using boost::asio::ip::udp;
int queues_table[ROW][COL] = {0};
int processed_table[ROW][COL] = {0};

social_network::json config_json;

std::string supervisor_db_addr;
int supervisor_db_port;

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
    // intialize database connection
    try {
      conn_ = new  pqxx::connection("postgresql://postgres:postgres@"+supervisor_db_addr+":"+std::to_string(supervisor_db_port)+"/postgres");
      conn_->prepare("insert_to_thrift_events", "insert into thrift_events (event_type, logged_at, sender_id, receiver_id, processed_count) values ($1, $2, $3, $4, $5)");
    } catch (const std::exception &e) {
      std::cout << "Error on DB connect";
      std::cerr << e.what() << std::endl;
    }
    start_receive();
  }

private:
  pqxx::connection* conn_;

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

    // Get current timestamp
    struct timeval time_now{};
    gettimeofday(&time_now, nullptr);
    std::time_t time_ms = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);

    // Extract the service name
    boost::tokenizer<boost::char_separator<char>> tok(msg, boost::char_separator<char>(":"));

    std::vector<std::string> tokens;
    std::copy(tok.begin(), tok.end(), std::back_inserter(tokens));

    // A valid message should consist of two tokens: the service
    // name and the name of the method to call.

    if (tokens.size() == 4) {
      int processed_msgs = std::stoi(tokens[0]);
      std::vector<std::string> proc_service_tokens, sender_service_tokens;
      std::cout << "4 Tokens" << std::endl;

      // Extract processor-service index
      boost::tokenizer<boost::char_separator<char>> proc_index_tok(tokens[2], boost::char_separator<char>("-"));
      std::copy(proc_index_tok.begin(), proc_index_tok.end(), std::back_inserter(proc_service_tokens));
      int proc_index = std::stoi(proc_service_tokens[1])-1;
      // Extract sender-service index
      boost::tokenizer<boost::char_separator<char>>  sender_index_tok(tokens[3], boost::char_separator<char>("-"));
      std::copy(sender_index_tok.begin(), sender_index_tok.end(), std::back_inserter(sender_service_tokens));
      int sender_index = std::stoi(sender_service_tokens[1])-1;
      std::cout << "Pre-update" << std::endl;
      // update queue entry
      mtx_.lock();
      queues_table[sender_index][proc_index] = queues_table[sender_index][proc_index]  
                                                - std::max(0, processed_msgs - (int)processed_table[proc_index][sender_index]);
      
      // update processed table
      processed_table[proc_index][sender_index] = std::max((int)processed_table[proc_index][sender_index], processed_msgs);
      mtx_.unlock();
      std::cout << "Post-update" << std::endl;

      // Add api call to the database(log)
      try {
        pqxx::work txn_0(*conn_);
        txn_0.prepared("insert_to_thrift_events")(0)(time_ms)(tokens[2])(tokens[3])(processed_table[proc_index][sender_index]).exec();
        txn_0.commit();
        std::cout << "Event insert successful" << std::endl;
      } catch (const std::exception &e) {
        std::cout << "Error on DB connect" << std::endl;
        std::cerr << e.what() << std::endl;
      }
      // Print queue and proccessed calls tables
      // mtx_.lock();
      // std::cout << "Q" << std::endl;
      // printMatrix(queues_table);
      // std::cout << "P" << std::endl;
      // printMatrix(processed_table);
      // mtx_.unlock();

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

      // Add api call to the database(log)
      try {
        pqxx::work txn_1(*conn_);
        txn_1.prepared("insert_to_thrift_events")(1)(time_ms)(tokens[1])(tokens[2])(NULL).exec();
        txn_1.commit();
        std::cout << "Event insert successful" << std::endl;
      } catch (const std::exception &e) {
        std::cout << "Error on DB connect" << std::endl;
        std::cerr << e.what() << std::endl;
      }

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
  {  }

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
    std::cout << "Error for ADD";
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
    std::cout << "Error for PROC";
    std::cerr << e.what() << std::endl;
  }
}

void db_worker_proc() {

};


int main()
{

  if (social_network::load_config_file("config/service-config.json", &config_json) != 0) {
    exit(EXIT_FAILURE);
  }

  supervisor_db_addr = config_json["supervisor-database"]["addr"];
  supervisor_db_port = config_json["supervisor-database"]["port"];

  boost::thread add_t{add_thread}, rm_t{remove_thread};
  boost::thread db_manager();
  add_t.join();
  rm_t.join();
  
  return 0;
}
