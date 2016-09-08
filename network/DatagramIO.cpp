// Copyright (c) 2016-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Poco/URI.h>
#include <Poco/Net/DatagramSocket.h>
#include <iostream>

/***********************************************************************
 * |PothosDoc Datagram IO
 *
 * The datagram IO block binds or connects to a UDP socket
 * and provides input and output ports for either streams or packets.
 *
 * The input port 0 accepts all stream and input packets and
 * sends their raw bytes over UDP. Input streams are fragmented
 * to UDP MTU size. Packets are truncated to UDP MTU size.
 * Packet metadata, labels, and datatype are not preserved.
 *
 * The output port 0 produces streams of the specified data type
 * in the "STREAM" output mode. And produces packets of the specified
 * data type in the "PACKET" output mode.
 *
 * |category /Network
 * |keywords udp datagram packet network
 *
 * |param dtype[Data Type] The output data type.
 * Sets the data type of the output port and also of the buffer in packet mode.
 * |widget DTypeChooser(float=1,cfloat=1,int=1,cint=1,uint=1,cuint=1,dim=1)
 * |default "complex_float64"
 * |preview disable
 *
 * |param uri[URI] The bind or connection uri string.
 * |default "udt://192.168.10.2:1234"
 *
 * |param opt[Option] Control if the socket is a server (BIND) or client (CONNECT).
 * |option [Connect] "CONNECT"
 * |option [Bind] "BIND"
 * |default "BIND"
 *
 * |param mode[Mode] The output mode (stream or packets).
 * <ul>
 * <li>"STREAM" - Produce the received datagram as a sample stream.</li>
 * <li>"PACKET" - Preserve the datagram boundaries and produce Pothos::Packet.</li>
 * </lu>
 * |default "STREAM"
 * |option [Stream] "STREAM"
 * |option [Packet] "PACKET"
 *
 * |param mtu[MTU] The maximum size of a datagram payload in bytes.
 * |default 1472
 * |units bytes
 *
 * |factory /blocks/datagram_io(dtype)
 * |initializer setupSocket(uri, opt)
 * |setter setMode(mode)
 * |setter setMTU(mtu)
 **********************************************************************/
class DatagramIO : public Pothos::Block
{
public:
    static Block *make(const Pothos::DType &dtype)
    {
        return new DatagramIO(dtype);
    }

    DatagramIO(const Pothos::DType &dtype):
        _packetMode(false),
        _mtu(1472)
    {
        this->setupInput(0);
        this->setupOutput(0, dtype);
        this->registerCall(this, POTHOS_FCN_TUPLE(DatagramIO, setupSocket));
        this->registerCall(this, POTHOS_FCN_TUPLE(DatagramIO, setMode));
        this->registerCall(this, POTHOS_FCN_TUPLE(DatagramIO, setMTU));
    }

    ~DatagramIO(void)
    {
        _sock.close();
    }

    void setupSocket(const std::string &uri, const std::string &opt)
    {
        try
        {
            Poco::URI uriObj(uri);
            const Poco::Net::SocketAddress addr(uriObj.getHost(), uriObj.getPort());
            if (opt == "CONNECT") _sock.connect(addr);
            else if (opt == "BIND") _sock.bind(addr);
            else throw Pothos::FileException("DatagramIO::setupSocket("+uri+" -> "+opt+")", "unknown option");
        }
        catch (const Poco::Exception &ex)
        {
            throw Pothos::InvalidArgumentException("DatagramIO::setupSocket("+uri+" -> "+opt+")", ex.displayText());
        }
    }

    void setMode(const std::string &mode)
    {
        if (mode == "STREAM") _packetMode = false;
        else if (mode == "PACKET") _packetMode = true;
        else throw Pothos::FileException("DatagramIO::InvalidArgumentException("+mode+")", "unknown mode");
    }

    void setMTU(const size_t mtu)
    {
        _mtu = mtu;
    }

    void work(void)
    {
        
    }

private:
    Poco::Net::DatagramSocket _sock;
    bool _packetMode;
    size_t _mtu;
};

static Pothos::BlockRegistry registerDatagramIO(
    "/blocks/datagram_io", &DatagramIO::make);
