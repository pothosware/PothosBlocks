// Copyright (c) 2016-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Poco/Logger.h>
#include <complex>
#include <fstream>
#include <cerrno>

/***********************************************************************
 * |PothosDoc Text File Sink
 *
 * The text file sink reads input data from port 0 and writes it
 * into the output file in a delimited ascii string format.
 *
 * Note that this is not a high-performance block:
 * Conversion to string is not a fast operation
 * and it bloats the original size of the data many-fold.
 *
 * <h2>Stream input</h2>
 *
 * Streaming input buffers are converted to string with iostream formatting.
 * Each input element is output on its own line within the output file.
 * If an element is a vector of numbers, its elements will be comma-separated.
 * Labels are currently ignored by this implementation.
 *
 * <h2>Message input</h2>
 *
 * Each input message will be converted to string
 * using the Pothos::Object::toString() function
 * and written out to a line in the output file.
 *
 * <h2>Packet input</h2>
 *
 * If the message is specifically the Pothos::Packet type,
 * the metadata will be written to file like a message input,
 * and the payload will be written to the file like a stream buffer.
 * Packet labels are currently ignored by this implementation.
 *
 * |category /Sinks
 * |category /File IO
 * |keywords sink text ascii file
 *
 * |param path[File Path] The path to the output file.
 * |default ""
 * |widget FileEntry(mode=save)
 *
 * |factory /blocks/text_file_sink()
 * |setter setFilePath(path)
 **********************************************************************/
class TextFileSink : public Pothos::Block
{
public:
    static Block *make(void)
    {
        return new TextFileSink();
    }

    TextFileSink(void)
    {
        this->setupInput(0);
        this->registerCall(this, POTHOS_FCN_TUPLE(TextFileSink, setFilePath));
    }

    void setFilePath(const std::string &path)
    {
        _path = path;
        //file was open -> close old fd, and open this new path
        if (_file.is_open())
        {
            this->deactivate();
            this->activate();
        }
    }

    void activate(void)
    {
        if (_path.empty()) throw Pothos::FileException("TextFileSink", "empty file path");
        _file.open(_path.c_str());
        if (not _file)
        {
            poco_error_f3(Poco::Logger::get("TextFileSink"), "open(%s) failed -- %s(%d)", _path, std::string(strerror(errno)), errno);
        }
    }

    void deactivate(void)
    {
        _file.close();
    }

    void work(void)
    {
        auto in0 = this->input(0);

        if (in0->hasMessage())
        {
            const auto msg = in0->popMessage();
            if (msg.type() == typeid(Pothos::Packet))
            {
                const auto pkt = msg.extract<Pothos::Packet>();
                this->writeObject(Pothos::Object(pkt.metadata));
                this->writeBuffer(pkt.payload);
            }
            else
            {
                this->writeObject(msg);
            }
        }

        if (in0->elements() != 0)
        {
            this->writeBuffer(in0->buffer());
            in0->consume(in0->elements());
        }
    }

private:

    void writeObject(const Pothos::Object &obj)
    {
        if (not _file.good()) return;
        _file << obj.toString() << std::endl;
    }

    void writeBuffer(const Pothos::BufferChunk &buff)
    {
        if (not _file.good()) return;

        //all complex types, even complex integers are converted to complex doubles and written out
        if (buff.dtype.isComplex()) this->writeBuffer<std::complex<double>>(buff);

        //then convert all floating point types to doubles to write them out
        else if (buff.dtype.isFloat()) this->writeBuffer<double>(buff);

        //the remaining integer types are converted to long long to write out
        else this->writeBuffer<long long>(buff);
    }

    template <typename Type>
    void writeBuffer(const Pothos::BufferChunk &buff)
    {
        const auto outBuff = buff.convert(typeid(Type), buff.dtype.dimension());
        const Type *elems = outBuff;
        for (size_t i = 0; i < buff.elements(); i++)
        {
            for (size_t j = 0; j < buff.dtype.dimension(); j++)
            {
                _file << (*(elems++));
                if (j+1 == buff.dtype.dimension()) _file << std::endl;
                else _file << ", ";
            }
        }
    }

    std::ofstream _file;
    std::string _path;
};

static Pothos::BlockRegistry registerTextFileSink(
    "/blocks/text_file_sink", &TextFileSink::make);
