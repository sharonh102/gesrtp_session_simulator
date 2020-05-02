#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/algorithm.hpp>

using boost::asio::ip::tcp;

const std::string path_prefix = "../../../Sample_packets/Raw/";
const std::vector<std::string> requestVec = {"read_plc_memory_request","write_plc_memory_request","return_plc_request"};

template<size_t Size, class Container>
boost::array<typename Container::value_type, Size> as_array(const Container &cont)
{
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
    // the user should specify the server & port
    if (argc != 3)
    {
        std::cerr << "Usage: client <server-ip> <port>" << std::endl;
        return 1;
    }
    try {
        // Any program that uses asio need to have at least one io_service object
        boost::asio::io_service io_service;

        // Convert the server name that was specified as a parameter to the application, to a TCP endpoint.
        // To do this, we use an ip::tcp::resolver object.
        tcp::resolver resolver(io_service);

        // A resolver takes a query object and turns it into a list of endpoints.
        // We construct a query using the name of the server, specified in argv[1],
        // and the name of the service, in this case "daytime".
        tcp::resolver::query query(argv[1], argv[2]);

        // The list of endpoints is returned using an iterator of type ip::tcp::resolver::iterator.
        // A default constructed ip::tcp::resolver::iterator object can be used as an end iterator.
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        // Now we create and connect the socket.
        // The list of endpoints obtained above may contain both IPv4 and IPv6 endpoints,
        // so we need to try each of them until we find one that works.
        // This keeps the client program independent of a specific IP version.
        // The boost::asio::connect() function does this for us automatically.
        tcp::socket socket(io_service);

        /*
         *  another endpoint creation option
            boost::asio::ip::address ip_address(boost::asio::ip::address::from_string("192.168.2.103"));
            tcp::endpoint client_ep1(tcp::v4(), 5000);
            client_ep.address(ip_address);
         */

        // bind to specific ip address:
        tcp::endpoint client_ep(boost::asio::ip::address::from_string("192.168.2.103"), 18200);
        socket.open(boost::asio::ip::tcp::v4());
        boost::system::error_code ec;
        socket.bind(client_ep);

        // connect to the server:
        tcp::endpoint server_ep(boost::asio::ip::address::from_string("192.168.2.105"), boost::lexical_cast<unsigned>(argv[2]));
        socket.connect(server_ep, ec);

        // The connection is open. All we need to do now is read the response from the daytime service.
        //while (true)
            for (auto requestPath : requestVec)
            {
                // prepare the response message to send back to client
                std::vector<unsigned char> payload = loadPayload(path_prefix+requestPath);
                std::string payload_msg = {payload.begin(),payload.end()};

                boost::system::error_code error;

                // writing the message for current time
                boost::asio::write(socket, boost::asio::buffer(payload), error);

                auto printHex = [&](std::string s)
                {
                    for(auto &c : s) {
                        std::cout << std::hex << std::setw(2) <<
                                  std::setfill('0') << static_cast<unsigned short>(c&0xff) << " ";
                    }
                    std::cout.unsetf(std::ios::hex);
                };

                if (error == boost::asio::error::eof)
                    break; // Connection closed cleanly by peer.
                else if (!error) {
                    std::cout << "PLC sent: [";
                    printHex(payload_msg);
                    std::cout << "]" << std::endl;
                }
                else
                    throw boost::system::system_error(error); // Some other error.

                //Read response:
                // We use a boost::array to hold the received data.
                boost::array<char, 128> buf;

                // The boost::asio::buffer() function automatically determines
                // the size of the array to help prevent buffer overruns.
                size_t len = socket.read_some(boost::asio::buffer(buf), error);

                // When the server closes the connection,
                // the ip::tcp::socket::read_some() function will exit with the boost::asio::error::eof error,
                // which is how we know to exit the loop.
                if (error == boost::asio::error::eof) {
                    std::cout << "Connection closed cleanly by server.";
                    break; // Connection closed cleanly by peer.
                }
                else if (error)
                    throw boost::system::system_error(error); // Some other error.

                //print received buffer:
                std::cout << "PLC recv: [";
                std::cout.setf(std::ios::hex, std::ios::basefield);
                printHex(std::string(buf.data(),len));
                std::cout << "]" << std::endl;
                buf.assign(0);

                sleep(3);
            }
    }

    // handle any exceptions that may have been thrown.
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
