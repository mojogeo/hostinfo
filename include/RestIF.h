#pragma once
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <fstream>
#include <random>

#include "cpprest/json.h"
#include "cpprest/http_listener.h"
#include "cpprest/uri.h"
#include "cpprest/asyncrt_utils.h"

using namespace web;
using namespace http;
using namespace utility;
using namespace http::experimental::listener;

class RestIF
{
public:
  RestIF() {}
  RestIF(utility::string_t p_url, bool p_encrypt, std::string p_keyFile, std::string p_certFile, bool p_verbose);
  pplx::task<void> open() { return m_listener->open(); }
  pplx::task<void> close() { return m_listener->close(); }
  bool isUnsignedInt(const std::string& s);
private:
  std::string getUsageHTML();
  std::string executeCommand(const char* command);
  void handleGet(http_request message);
  void handlePut(http_request message);
  void handlePost(http_request message);
  void handleDelete(http_request message);
  std::shared_ptr<http_listener> m_listener;
  std::string m_url;
  std::string m_startTime;
  std::string m_certFile;
  std::string m_keyFile;
  bool m_verbose;
};
