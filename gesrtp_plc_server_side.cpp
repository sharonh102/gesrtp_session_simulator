#include <ctime>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <fstream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/algorithm.hpp>

using boost::asio::ip::tcp;

const std::string path_prefix = "../../../Sample_packets/Raw/";
const std::vector<std::string> responseVec = {"read_plc_memory_response","write_plc_memory_response","return_plc_response"};


template<size_t Size, class Container>
boost::array<typename Container::value_type, Size> as_array(const Container &cont)
{
    std::cout << "size = [" << cont.size() << "]=?= [" << Size << "]" << std::endl;
    assert(cont.size() == Size);
    boost::array<typename Container::value_type, Size> result;
    boost::range::copy(cont, result.begin());
    return result;
}

std::vector<unsigned char> loadPayload(std::string path)
{
    std::ifstream packetFile(path, std::ios::in | std::ios::binary);
    std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(packetFile)),
                                      std::istreambuf_iterator<char>());
    return buffer;
}

int main(int argc, char* argv[])
{
  // the user should specify the port - the 2nd argument
  if (argc != 2)
  {
    std::cerr << "Usage: server <port>" << std::endl;
    return 1;
  }

  // prepare the response message to send back to client
  std::vector<unsigned char> payload = loadPayload(path_prefix+"return_plc_response");
  std::string payload_msg = {payload.begin(),payload.end()};

  try
  {
    // Any program that uses asio need to have at least one io_service object
    boost::asio::io_service io_service;

    // bind to specific ip address:
    boost::asio::ip::address ip_address(boost::asio::ip::address::from_string("192.168.2.105"));
    tcp::endpoint endpoint(tcp::v4(), boost::lexical_cast<unsigned>(argv[1]));
    endpoint.address(ip_address);

    // acceptor object needs to be created to listen for new connections
    tcp::acceptor acceptor(io_service, endpoint);

    // creates a socket
    tcp::socket socket(io_service);

    // wait and listen
    acceptor.accept(socket);

    while(true)
        for (auto responsePath : responseVec)
        {
          // prepare the response message to send back to client
          std::vector<unsigned char> payload = loadPayload(path_prefix+responsePath);
          std::string payload_msg = {payload.begin(),payload.end()};

          // We use a boost::array to hold the received data.
          boost::array<char, 128> buf;
          boost::system::error_code error;

          // The boost::asio::buffer() function automatically determines
          // the size of the array to help prevent buffer overruns.
          size_t len = socket.read_some(boost::asio::buffer(buf), error);

          // When the server closes the connection,
          // the ip::tcp::socket::read_some() function will exit with the boost::asio::error::eof error,
          // which is how we know to exit the loop.
          if (error == boost::asio::error::eof)
              break; // Connection closed cleanly by peer.
          else if (error)
              throw boost::system::system_error(error); // Some other error.

           auto printHex = [&](std::string s)
                   {
                        for(auto &c : s) {
                            std::cout << std::hex << std::setw(2) <<
                            std::setfill('0') << static_cast<unsigned short>(c&0xff) << " ";
                        }
                       std::cout.unsetf(std::ios::hex);
                   };

           //print received buffer:
           std::cout << "Server: recv [";
           printHex(std::string(buf.data(),len));
           std::cout << "]" << std::endl;

           boost::system::error_code ignored_error;

           buf.assign(0);

           // writing the message for current time
           boost::asio::write(socket, boost::asio::buffer(payload), ignored_error);

           std::cout.setf(std::ios::hex, std::ios::basefield);
           std::cout << "Server sent: [";
           printHex(payload_msg);
           std::cout << "]" << std::endl;
        }
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
