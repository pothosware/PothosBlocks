// Copyright (c) 2016-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Poco/URI.h>
#include <Poco/Logger.h>
#include <Poco/Net/DatagramSocket.h>
#include <algorithm> //min/max
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
 * |default "udp://localhost:1234"
 * |widget StringEntry()
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
        _logger(Poco::Logger::get("DatagramIO")),
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
            else if (opt == "BIND") _sock.bind(addr, true/*reuse*/);
            else throw Pothos::FileException("DatagramIO::setupSocket("+uri+" -> "+opt+")", "unknown option");
        }
        catch (const Poco::Exception &ex)
        {
            throw Pothos::InvalidArgumentException("DatagramIO::setupSocket("+uri+" -> "+opt+")", ex.displayText());
        }
        _socketConnected = (opt == "CONNECT");
    }

    void setMode(const std::string &mode)
    {
        if (mode == "STREAM") _packetMode = false;
        else if (mode == "PACKET") _packetMode = true;
        else throw Pothos::FileException("DatagramIO::InvalidArgumentException("+mode+")", "unknown mode");
    }

    void setMTU(const size_t mtu)
    {
        auto outPort = this->output(0);
        const size_t elemSize = outPort->dtype().size();
        if ((mtu % elemSize) != 0) throw Pothos::InvalidArgumentException("DatagramIO::setMTU("+std::to_string(mtu)+")",
            "The MTU is not a multiple of the output data-type size: " + outPort->dtype().toString());

        outPort->setReserve(_mtu/elemSize);
        _mtu = mtu;
    }

    void work(void)
    {
        auto inPort = this->input(0);
        bool hadEvent = false;

        //incoming packet to send
        if (inPort->hasMessage())
        {
            const auto msg = inPort->popMessage();
            if (msg.type() != typeid(Pothos::Packet))
            {
                poco_error_f1(_logger, "Dropped input message of type %s; only Pothos::Packet supported", msg.getTypeString());
            }
            const auto &pkt = msg.extract<Pothos::Packet>();
            this->sendBuffer(pkt.payload);
            hadEvent = true;
        }

        //incoming stream to send (clip to MTU)
        auto inBuff = inPort->buffer();
        if (inBuff.length != 0)
        {
            //clip to the MTU size
            inBuff.length = std::min(inBuff.length, _mtu);

            //preserve element multiples
            const size_t elemSize = inBuff.dtype.size();
            inBuff.length = (inBuff.length/elemSize)*elemSize;

            inPort->consume(inBuff.length);
            this->sendBuffer(inBuff);
            hadEvent = true;
        }

        //small polling sleep if nothing happened and there is nothing to recv
        if (not hadEvent and _sock.available() == 0)
        {
            const auto pollTimeUs = std::min<Poco::Timespan::TimeDiff>(10, this->workInfo().maxTimeoutNs/1000);
            _sock.poll(Poco::Timespan(pollTimeUs), Poco::Net::Socket::SELECT_READ);
        }

        //incoming UDP datagram
        if (_sock.available() != 0)
        {
            auto outPort = this->output(0);
            auto outBuff = outPort->buffer();
            try
            {
                Poco::Net::SocketAddress recvAddr;
                int ret = _sock.receiveFrom(outBuff.as<void *>(), outBuff.length, recvAddr);
                if (ret > 0)
                {
                    if ((ret % outBuff.dtype.size()) != 0)
                    {
                        poco_warning_f2(_logger,
                            "Received %d bytes is not a multiple of the output size: %s.\n"
                            "Until the sender is fixed, expect possible truncation of data.",
                            ret, outBuff.dtype.toString());
                    }

                    outBuff.length = ret;
                    if (_packetMode)
                    {
                        Pothos::Packet pkt;
                        pkt.payload = outBuff;
                        outPort->popElements(outBuff.elements());
                        outPort->postMessage(pkt);
                    }
                    else
                    {
                        outPort->produce(outBuff.elements());
                    }

                    //the new send-to address for bound sockets
                    if (not _socketConnected) _sendAddr = recvAddr;
                }
                else
                {
                    poco_error_f2(_logger, "Socket recv %d bytes failed: ret = %d", int(outBuff.length), ret);
                }
            }
            catch (const Poco::Exception &ex)
            {
                poco_error_f2(_logger, "Socket recv %d bytes failed: %s", int(outBuff.length), ex.displayText());
            }
        }
    }

private:

    void sendBuffer(const Pothos::BufferChunk &buff)
    {
        try
        {
            int ret = 0;
            if (_socketConnected) ret = _sock.sendBytes(buff.as<const void *>(), buff.length);
            else ret = _sock.sendTo(buff.as<const void *>(), buff.length, _sendAddr);

            if (ret != int(buff.length))
            {
                poco_error_f2(_logger, "Socket send %d bytes failed: ret = %d", int(buff.length), ret);
            }
        }
        catch (const Poco::Exception &ex)
        {
            if (not _socketConnected and _sendAddr == Poco::Net::SocketAddress())
            {
                poco_error(_logger, "A bound socket cannot send until it has received!");
            }
            else
            {
                poco_error_f2(_logger, "Socket send %d bytes failed: %s", int(buff.length), ex.displayText());
            }
        }
    }

    Poco::Logger &_logger;
    Poco::Net::DatagramSocket _sock;
    bool _packetMode;
    size_t _mtu;

    //bound sockets only send to the last received address
    bool _socketConnected;
    Poco::Net::SocketAddress _sendAddr;
};

static Pothos::BlockRegistry registerDatagramIO(
    "/blocks/datagram_io", &DatagramIO::make);
