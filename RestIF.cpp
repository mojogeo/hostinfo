#include "RestIF.h"

using namespace web; 
using namespace utility;
using namespace http;
using namespace web::http::experimental::listener;

std::string RestIF::executeCommand(const char* command) 
{
  std::array<char, 128> buffer;
  std::string result;
  std::shared_ptr<FILE> pipe(popen(command, "r"), pclose);
  if (!pipe) throw std::runtime_error("popen() failed!");
  while (!feof(pipe.get()))
  {
    if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
      result += buffer.data();
  }
  return result;
}

RestIF::RestIF(utility::string_t p_url, bool p_encrypt, std::string p_keyFile, std::string p_certFile, bool p_verbose) 
    : m_url(p_url), m_verbose(p_verbose), m_certFile(p_certFile), m_keyFile(p_keyFile)
{
  if(m_verbose)
    std::cout << "Starting REST interface" << std::endl;

  if(p_encrypt)
  {
    if(m_verbose)
    {
      std::cout << "Mode: HTTPS - encrypted" << std::endl;
      std::cout << "Private key file: " << p_keyFile << "\nCertificate file: " << p_certFile << std::endl;
    }
    
    http_listener_config config;
    boost::system::error_code ec;
    config.set_ssl_context_callback(
      [&](boost::asio::ssl::context& ctx)
      {
        ctx.set_options(boost::asio::ssl::context::default_workarounds,ec);
        if(ec)
        {
          std::cerr << "ERROR while setting encryption options: " << ec.message() << std::endl;
          return;
        }
        // NOTE: To generate self-signed key/cert:
        // openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days 365 -out cert.pem
        ctx.use_certificate_file(m_certFile.c_str(), boost::asio::ssl::context::pem, ec);
        if(ec)
        {
          std::cerr << "ERROR while loading cert file " << p_certFile << ": " << ec.message() << std::endl;
          return;
        }
        ctx.use_private_key_file(m_keyFile.c_str(), boost::asio::ssl::context::pem, ec);
        if(ec)
        {
          std::cerr << "ERROR while loading key file " << p_keyFile << ": " << ec.message() << std::endl;
          return;
        }
      });
      
    m_listener = std::shared_ptr<http_listener>(new http_listener(p_url, config));
  }
  else // HTTP, no encryption
  {
    if(m_verbose)
      std::cout << "Mode: HTTP - not encrypted" << std::endl;
    m_listener = std::shared_ptr<http_listener>(new http_listener(p_url));
  }

  m_listener->support(methods::GET,  std::bind(&RestIF::handleGet,    this, std::placeholders::_1));
  m_listener->support(methods::PUT,  std::bind(&RestIF::handlePut,    this, std::placeholders::_1));
  m_listener->support(methods::POST, std::bind(&RestIF::handlePost,   this, std::placeholders::_1));
  m_listener->support(methods::DEL,  std::bind(&RestIF::handleDelete, this, std::placeholders::_1));

  // Save start time as a string
  time_t now;
  struct tm *ts;
  char buf[80];
  now = time(NULL);
  ts = localtime(&now);
  strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S", ts);
  m_startTime = buf;
}

bool RestIF::isUnsignedInt(const std::string& s)
{
  return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

std::string RestIF::getUsageHTML()
{
  std::stringstream html;

  // HTML opening tags, etc
  html << "<!doctype html>\n<html>\n<head>\n<title>hostinfo</title>\n";
  html << "<style>table, th, td { border: 1px solid black; border-collapse: collapse; }</style>\n";
  html << "</head><body>\n";
  html << "<h1>Host Information</h1>\n";
  html << "<p>Provided by hostinfo service</p>\n";

  // Some service information
  html << "<h2>Service information</h2>\n";
  html << "<ul>\n";
  html << "<li>URL: " << m_url << "</li>\n";
  html << "<li>Start time: " << m_startTime << "</li>\n";
  html << "</ul>\n";

  // API call paths
  html << "<h2>Application Program Interface (API)</h2>\n"
    << "<table style=\"width:100%\"><tr><th style=\"width:25%\">Path</th><th style=\"width:75%\">Notes</th></tr>\n"
    << "<tr><td><a href=\"service/info\">service/info</a></td><td>Information/test page</td></tr>\n"
    << "<tr><td><a href=\"hardware\">hardware</a></td><td>Get list of hardware reported by lshw</td></tr>\n"
    << "<tr><td><a href=\"processes?LimitResults=10\">processes?LimitResults=10</a></td><td>Get list of processes reported by top (optionally limited by \"LimitResults\" parameter)</td></tr>\n"
    << "<tr><td><a href=\"packages?LimitResults=10\">packages?LimitResults=10</a></td><td>Get list of software packages reported by rpm (optionally limited by \"LimitResults\" parameter)</td></tr>\n"
    << "<tr><td><a href=\"sysinfo\">sysinfo</a></td><td>Get list of system information reported by uname</td></tr>\n"
    << "</table>\n";

  // HTML closing tags
  html << "</body></html>";
  return html.str();
}

void RestIF::handleGet(http_request message)
{
  if(m_verbose)
    std::cout << "Handling GET request: " << message.to_string() << std::endl;

  auto relativePath = uri::decode(message.relative_uri().path());
  auto path = uri::split_path(relativePath);
  auto queryVars = uri::split_query(message.request_uri().query());

  if (!path.empty()) 
  {
    if (path[0] == "service" && path[1] == "info") 
    {
      auto response = json::value::object();
      response["name"] = json::value::string("hostinfo");
      response["version"] = json::value::string("0");
      response["status"] = json::value::string("ready");
      message.reply(status_codes::OK, response);
    }
    else if (path.size()==1 && path[0] == "hardware")
    {
      // Reply with results of "lshw -json"
      message.reply(status_codes::OK, json::value::parse(executeCommand("lshw -json")));
      std::string results = executeCommand("lshw -json");
      //std::cout << "results: " << results << std::endl;
      message.reply(status_codes::OK, json::value::parse(results));
    }
    else if (path.size()==1 && path[0] == "processes")
    {
      std::string command("top -bn 1 | grep \"^ \" | tail -n +2");
      if(queryVars.count("LimitResults") && isUnsignedInt(queryVars["LimitResults"]))
        command += " | head -n" + queryVars["LimitResults"];
      command += " | awk '{ printf(\"\\\"%s\\\" : {\\\"PercentCPU\\\" : \\\"%s\\\", \\\"PercentMem\\\" : \\\"%s\\\", \\\"Command\\\" : \\\"%s\\\"},\\n\", $1, $9, $10, $12); }'";
      std::string results = std::string("{\n") + executeCommand(command.c_str());
      results.pop_back(); // remove last new line character
      results.pop_back(); // remove last comma
      results += std::string("\n}");
      //std::cout << "results: " << results << std::endl;
      message.reply(status_codes::OK, json::value::parse(results));
    }
    else if (path.size()==1 && path[0] == "packages")
    {
      std::string command("rpm -qa | sort");
      if(queryVars.count("LimitResults") && isUnsignedInt(queryVars["LimitResults"]))
        command += " | head -n" + queryVars["LimitResults"];
      command += " | awk '{ printf(\"{ \\\"RPM\\\" : \\\"%s\\\" },\\n\", $1); }'";
      std::string results = std::string("{\n\"Packages\" : [\n") + executeCommand(command.c_str());
      results.pop_back(); // remove last new line character
      results.pop_back(); // remove last comma
      results += std::string("\n]\n}");
      //std::cout << "results: " << results << std::endl;
      message.reply(status_codes::OK, json::value::parse(results));
    }
    else if (path.size()==1 && path[0] == "sysinfo")
    {
      std::string kernelNameJSON    = std::string("{ \"KernalName\" : \"")       + executeCommand("uname --kernel-name")       + std::string("\" },");
      std::string hostNameJSON      = std::string("{ \"HostName\" : \"")         + executeCommand("uname --nodename")          + std::string("\" },");
      std::string kernelReleaseJSON = std::string("{ \"KernelRelease\" : \"")    + executeCommand("uname --kernel-release")    + std::string("\" },");
      std::string kernelVersionJSON = std::string("{ \"KernelVersion\" : \"")    + executeCommand("uname --kernel-version")    + std::string("\" },");
      std::string machineJSON       = std::string("{ \"Machine\" : \"")          + executeCommand("uname --machine")           + std::string("\" },");
      std::string processorJSON     = std::string("{ \"Processor\" : \"")        + executeCommand("uname --processor")         + std::string("\" },");
      std::string hwPlatformJSON    = std::string("{ \"HardwarePlatform\" : \"") + executeCommand("uname --hardware-platform") + std::string("\" },");
      std::string osJSON            = std::string("{ \"OperatingSystem\" : \"")  + executeCommand("uname --operating-system")  + std::string("\" }");
      std::string results = std::string("{ \"SysInfo\" : [ ")
        + kernelNameJSON + hostNameJSON + kernelReleaseJSON + kernelVersionJSON + machineJSON + processorJSON + hwPlatformJSON + osJSON + " ] }";
      results.erase(std::remove(results.begin(), results.end(), '\n'), results.end()); // Remove new lines that uname adds
      //std::cout << "results: " << results << std::endl;
      message.reply(status_codes::OK, json::value::parse(results));
    }
   else
    {
      message.reply(status_codes::NotFound, "Path not found");
    }
  }
  else
  {
    message.reply(status_codes::OK,getUsageHTML(),"text/html;charset=utf-8");
  }
}

void RestIF::handlePost(http_request message)
{
  // (Just a stub)
  //std::cout <<  message.to_string() << std::endl;
  message.reply(status_codes::OK);
}

void RestIF::handleDelete(http_request message)
{
  // (Just a stub)
  //std::cout << message.to_string() << std::endl;
  message.reply(status_codes::OK);
}

void RestIF::handlePut(http_request message)
{
  // (Just a stub)
  //std::cout << message.to_string() << std::endl;
  message.reply(status_codes::OK);
}