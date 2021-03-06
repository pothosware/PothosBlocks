// Copyright (c) 2014-2017 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "SocketEndpoint.hpp"
#include <Pothos/Framework.hpp>
#include <cstring> //std::memset
#include <sstream>
#include <string>
#include <cassert>
#include <iostream>

/***********************************************************************
 * |PothosDoc Network Source
 *
 * The network source deserializes data from the socket and produces on its output port.
 * Socket data encompasses stream buffers, inline labels, and async messages.
 *
 * The underlying supports the tcp transport option:
 * TCP - tcp://host:port
 *
 * |category /Network
 * |category /Sources
 * |keywords source network
 *
 * |param uri[URI] The bind or connection uri string.
 * |default "tcp://192.168.10.2:1234"
 *
 * |param opt[Option] Control if the socket is a server (BIND) or client (CONNECT).
 * The "DISCONNECT" option is used to make a disconnected endpoint for object inspection.
 * |option [Disconnect] "DISCONNECT"
 * |option [Connect] "CONNECT"
 * |option [Bind] "BIND"
 * |default "DISCONNECT"
 *
 * |factory /blocks/network_source(uri, opt)
 **********************************************************************/
class NetworkSource : public Pothos::Block
{
public:
    static Block *make(const std::string &uri, const std::string &opt)
    {
        return new NetworkSource(uri, opt);
    }

    NetworkSource(const std::string &uri, const std::string &opt):
        _ep(PothosPacketSocketEndpoint(uri, opt))
    {
        //std::cout << "NetworkSource " << opt << " " << uri << std::endl;
        this->setupOutput(0);
        this->registerCall(this, POTHOS_FCN_TUPLE(NetworkSource, getActualPort));
    }

    std::string getActualPort(void) const
    {
        return _ep.getActualPort();
    }

    void activate(void)
    {
        _ep.openComms();
    }

    void deactivate(void)
    {
        _ep.closeComms();
    }

    void work(void);

private:
    PothosPacketSocketEndpoint _ep;
    Pothos::DType _lastDtype;
    Pothos::Packet _packetHeader;
};

void NetworkSource::work(void)
{
    const auto timeoutNanos = std::chrono::nanoseconds(this->workInfo().maxTimeoutNs);
    const auto timeout = std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(timeoutNanos);

    auto outputPort = this->output(0);

    //recv the header, use output buffer when possible for zero-copy
    uint16_t type = 0;
    auto buffer = outputPort->buffer();
    _ep.recv(type, buffer, timeout);

    //handle the output
    if (type == PothosPacketTypeBuffer)
    {
        buffer.dtype = _lastDtype;
        outputPort->popElements(buffer.length);
        outputPort->postBuffer(std::move(buffer));
    }
    else if (type == PothosPacketTypeMessage)
    {
        std::istringstream iss(std::string(buffer.as<char *>(), buffer.length));
        Pothos::Object msg;
        msg.deserialize(iss);
        outputPort->postMessage(std::move(msg));
    }
    else if (type == PothosPacketTypeHeader)
    {
        std::istringstream iss(std::string(buffer.as<char *>(), buffer.length));
        Pothos::Object msg; msg.deserialize(iss);
        _packetHeader = std::move(msg.ref<Pothos::Packet>()); //store it, payload comes next
    }
    else if (type == PothosPacketTypePayload)
    {
        //since this is not PothosPacketTypeBuffer, recv may have allocated a new buffer
        //only pop if this is really the buffer from the output port
        if (buffer.address == outputPort->buffer().address) outputPort->popElements(buffer.length);

        buffer.dtype = _lastDtype;
        _packetHeader.payload = buffer;
        outputPort->postMessage(std::move(_packetHeader));
    }
    else if (type == PothosPacketTypeLabel)
    {
        std::istringstream iss(std::string(buffer.as<char *>(), buffer.length));
        Pothos::Object data;
        data.deserialize(iss);
        auto &label = data.ref<Pothos::Label>();
        outputPort->postLabel(std::move(label));
    }
    else if (type == PothosPacketTypeDType)
    {
        std::istringstream iss(std::string(buffer.as<char *>(), buffer.length));
        Pothos::Object data;
        data.deserialize(iss);
        _lastDtype = std::move(data.ref<Pothos::DType>());
    }

    return this->yield(); //always yield to service recv() again
}

static Pothos::BlockRegistry registerNetworkSource(
    "/blocks/network_source", &NetworkSource::make);
