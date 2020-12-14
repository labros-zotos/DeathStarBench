#ifndef SOCIAL_NETWORK_MICROSERVICES_TRACEDCLIENTPOOL_H
#define SOCIAL_NETWORK_MICROSERVICES_TRACEDCLIENTPOOL_H

#include <vector>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <chrono>
#include <string>

#include "logger.h"

namespace social_network {

template<class TClient>
class TracedClientPool {
 public:
  TracedClientPool(const std::string &client_type, const std::string &addr,
      int port, int min_size, int max_size, int timeout_ms, const std::string &service_id, const std::string &receiver_id);
  ~TracedClientPool();

  TracedClientPool(const TracedClientPool&) = delete;
  TracedClientPool& operator=(const TracedClientPool&) = delete;
  TracedClientPool(TracedClientPool&&) = default;
  TracedClientPool& operator=(TracedClientPool&&) = default;

  TClient * Pop();
  void Push(TClient *);
  void Push(TClient *, int);
  void Remove(TClient *);

 private:
  std::deque<TClient *> _pool;
  std::string _addr;
  std::string _client_type;
  int _port;
  int _min_pool_size{};
  int _max_pool_size{};
  int _curr_pool_size{};
  int _timeout_ms;
  std::mutex _mtx;
  std::condition_variable _cv;

  std::string _service_id;
  std::string _receiver_id;

};

template<class TClient>
TracedClientPool<TClient>::TracedClientPool(const std::string &client_type,
    const std::string &addr, int port, int min_pool_size,
    int max_pool_size, int timeout_ms, const std::string &service_id, const std::string &receiver_id) {
  _addr = addr;
  _port = port;
  _min_pool_size = min_pool_size;
  _max_pool_size = max_pool_size;
  _timeout_ms = timeout_ms;
  _client_type = client_type;

  _service_id = service_id;
  _receiver_id = receiver_id;

  for (int i = 0; i < min_pool_size; ++i) {
    TClient *client = new TClient(addr, port, service_id, receiver_id);
    _pool.emplace_back(client);
  }
  _curr_pool_size = min_pool_size;
}

template<class TClient>
TracedClientPool<TClient>::~TracedClientPool() {
  while (!_pool.empty()) {
    delete _pool.front();
    _pool.pop_front();
  }
}

template<class TClient>
TClient * TracedClientPool<TClient>::Pop() {
  TClient * client = nullptr;
  std::unique_lock<std::mutex> cv_lock(_mtx); {
    while (_pool.size() == 0) {
      // Create a new a client if current pool size is less than
      // the max pool size.
      if (_curr_pool_size < _max_pool_size) {
        try {
          client = new TClient(_addr, _port, _service_id, _receiver_id);
          _curr_pool_size++;
          break;
        } catch (...) {
          cv_lock.unlock();
          return nullptr;
        }
      } else {
        auto wait_time = std::chrono::system_clock::now() +
            std::chrono::milliseconds(_timeout_ms);
        bool wait_success = _cv.wait_until(cv_lock, wait_time,
            [this] { return _pool.size() > 0; });
        if (!wait_success) {
          LOG(warning) << "TracedClientPool pop timeout";
          cv_lock.unlock();
          return nullptr;
        }
      }
    }
    if (!client){
      client = _pool.front();
      _pool.pop_front();
    }

  } // cv_lock(_mtx)
  cv_lock.unlock();

  if (client) {
    try {
      client->Connect();
    } catch (...) {
      LOG(error) << "Failed to connect " + _client_type;
      _pool.push_back(client);
      throw;
    }    
  }
  return client;
}

template<class TClient>
void TracedClientPool<TClient>::Push(TClient *client) {
  std::unique_lock<std::mutex> cv_lock(_mtx);
  client->KeepAlive();
  _pool.push_back(client);
  cv_lock.unlock();
  _cv.notify_one();
}

template<class TClient>
void TracedClientPool<TClient>::Push(TClient *client, int timeout_ms) {
  std::unique_lock<std::mutex> cv_lock(_mtx);
  client->KeepAlive(timeout_ms);
  _pool.push_back(client);
  cv_lock.unlock();
  _cv.notify_one();
}

template<class TClient>
void TracedClientPool<TClient>::Remove(TClient *client) {
  std::unique_lock<std::mutex> lock(_mtx);
  delete client;
  _curr_pool_size--;
  lock.unlock();
}

} // namespace social_network


#endif //SOCIAL_NETWORK_MICROSERVICES_TRACEDCLIENTPOOL_H