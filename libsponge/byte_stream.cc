#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : _error(false), _end(false), cap(capacity), byte_written(0), byte_read(0), buf(std::deque<char>()) {}

size_t ByteStream::write(const string &data) {
    size_t len = 0;
    for (auto c : data) {
        if (buf.size() == cap) {
            return len;
        }
        buf.push_back(c);
        len++;
        byte_written++;
    }
    return len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string result;
    for (size_t i = 0; i < len && i <= buf.size(); i++) {
        auto c = buf.at(i);
        result.push_back(static_cast<char>(c));
    }
    return result;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    for (size_t i = 0; i < len && !buf.empty(); i++, byte_read++) {
        buf.pop_front();
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    std::string result;
    for (size_t i = 0; i < len && !buf.empty(); i++, byte_read++) {
        char c = buf.front();
        result.push_back(c);
        buf.pop_front();
    }
    return result;
}

void ByteStream::end_input() { _end = true; }

bool ByteStream::input_ended() const { return _end; }

size_t ByteStream::buffer_size() const { return buf.size(); }

bool ByteStream::buffer_empty() const { return buf.empty(); }

bool ByteStream::eof() const { return _end && buf.empty(); }

size_t ByteStream::bytes_written() const { return byte_written; }

size_t ByteStream::bytes_read() const { return byte_read; }

size_t ByteStream::remaining_capacity() const { return cap - buf.size(); }
