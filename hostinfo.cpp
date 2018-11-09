// TODO: Determine if all of these includes are needed (this project was started from another project that needed them). 
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <thread>
#include "RestIF.h"
#include "argh.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

bool isFile(std::string p_path)
{
  bool isFile = false;
  try
  {
    isFile = boost::filesystem::is_regular_file(
      boost::filesystem::canonical(p_path));
  }
  catch(...)
  {
    return false;
  }
  return isFile;
}

std::atomic<bool> exitRequested(false);
void signalHandler(int sigNum)
{
  std::cout << "Caught signal: " << sigNum << std::endl;
  exitRequested = true;
}

// REST interface (HTTP/JSON)
std::unique_ptr<RestIF> g_restIF;

void initRestIF(const string_t& p_address, bool p_encrypt, std::string p_keyFile, std::string p_certFile, bool p_verbose)
{
  uri_builder uri(p_address);
  auto addr = uri.to_uri().to_string();
  g_restIF = std::unique_ptr<RestIF>(new RestIF(addr, p_encrypt, p_keyFile, p_certFile, p_verbose));
  g_restIF->open().wait();
  return;
}

void printUsage()
{
  std::cout << "\nUsage: hostinfo ..."
    << "\nOptions:"
    << "\n  -h [HOST], --host [host]       IP host to listen on"
    << "\n                                 (default: localhost)."
    << "\n  -p [PORT], --port [port]       IP port to listen on"
    << "\n                                 (1025 to 65535, default: 55111)."
    << "\n                                 (Binding to 80 or 443 if permitted)."
    << "\n  -e, --encrypt                  Use encryption with HTTPS."
    << "\n  -k [FILE], --key [FILE]        Private key file location for HTTPS"
    << "\n                                 (default: key.pem)."
    << "\n  -c [FILE], --cert [FILE]       Certificate chain file location for HTTPS"
    << "\n                                 (default: cert.pem)."
    << "\n  -v, --verbose                  Provide additional information."
    << "\n  -h, --help                     Print this help message and quit."
    << std::endl;
}

int main( int argc, char * argv[] )
{
  // Handle signals
  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = signalHandler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  sigaction(SIGINT, &sigIntHandler, NULL);

  // Option parsing variables
  argh::parser cmdl;
  cmdl.parse(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION);
  bool foundBadArg = false;

  // User options with default values
  bool verbose = false;
  std::string host("localhost");
  std::string port("55111");
  bool encrypt = false;
  std::string keyFile("key.pem");
  std::string certFile("cert.pem");

  // Positional arguments (none of these defined)
  bool firstSkipped = false;
  for (auto& posArg : cmdl)
  {
      if(!firstSkipped)
      {
        firstSkipped = true;
        continue; // Skipping app name
      }
      foundBadArg = true;
      std::cout << "Unrecognized argument \"" << posArg << "\"" << std::endl;    
  }

  // Parameters
  for (auto& param : cmdl.params())
  {
    if(param.first=="h" || param.first=="host")
    {
      // TODO: Provide some validation for host name/IP?
      host = param.second;
    }
    else if(param.first=="k" || param.first=="key")
    {
      keyFile = param.second;
      if( !isFile(keyFile) )
      {
        std::cerr << "ERROR: Could not open key file " << keyFile << std::endl;
        return EXIT_FAILURE;
      }
    }
    else if(param.first=="c" || param.first=="cert")
    {
      certFile = param.second;
      if( !isFile(certFile) )
      {
        std::cerr << "ERROR: Could not open cert file " << certFile << std::endl;
        return EXIT_FAILURE;
      }
    }
    else if(param.first=="p" || param.first=="port")
    {
      try
      {
        port = param.second;
        uint64_t portLL = std::stoll(port);
        // Note: only root user can bind to HTTP/80 or HTTPS/443
        if((portLL<1025 || portLL>65535) && portLL!=80 && portLL!=443)
        {
          foundBadArg = true;
          std::cout << "Bad listen port parameter value (\"" << param.second 
            << "\"). Set an integer value for port between 1025 and 65535."
            << std::endl;
        }
     }
      catch (std::invalid_argument e)
      {
        foundBadArg = true;
        std::cout << "Bad listen port parameter value (\"" << param.second 
           << "\"). Set an integer value for port between 1025 and 65535."
           << std::endl;
     }
      catch (std::out_of_range e)
      {
        foundBadArg = true;
        std::cout << "Bad listen port parameter value (\"" << param.second 
           << "\"). Set an integer value for port between 1025 and 65535."
           << std::endl;
      }
      catch (...)
      {
        std::cout << "Bad listen port parameter value (\"" << param.second 
           << "\"). Set an integer value for port between 1025 and 65535."
           << std::endl;
        return EXIT_FAILURE;
      }
    }
    else
    {
      foundBadArg = true;
      std::cout << "Unrecognized parameter \"" << param.first << "\": \"" 
        << param.second << "\"" << std::endl;
    }
  }

  // Flags
  for (auto& flag : cmdl.flags())
  {
    if(flag=="v" || flag=="verbose")
    {
      verbose = true;
    }
    else if(flag=="h" || flag=="help")
    {
      printUsage();
      return EXIT_SUCCESS;
    }
    else if(flag=="e" || flag=="encrypt")
    {
      encrypt = true;
    }
    else
    {
      foundBadArg = true;
      std::cout << "Unrecognized flag \"" << flag << "\"" << std::endl;
    }
  }

  if (foundBadArg)
  {
    printUsage();
    return EXIT_FAILURE;
  }

  // URL for HTTP or HTTPS
  std::string protocol("http");
  if(encrypt)
    protocol = "https";
  std::string address(protocol + "://" + host + ":");
  
  std::cout << "\nStarting hostinfo" << std::endl;
 
  // TODO: Put this in a try/catch block (catch bad address/port settings)
  address.append(port);
  std::cout << "Initializing REST interface on " << address << std::endl;
  initRestIF(address, encrypt, keyFile, certFile, verbose);

  try
  {
    std::chrono::high_resolution_clock::time_point t1(std::chrono::seconds(0));
    std::chrono::high_resolution_clock::time_point t2;
    while(true)
    {
      if(exitRequested)
      {
        std::cout << "Exiting hostinfo service" << std::endl;
        break;
      }
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
  catch(std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  catch( ... )
  {
    std::cerr << "Unknown exception" << std::endl;
    return EXIT_FAILURE;
  }

  g_restIF->close().wait();
  return EXIT_SUCCESS;
}
