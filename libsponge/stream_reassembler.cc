#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _capacity(capacity), _buf(std::map<size_t, char>()), _index(0), _eof_index(-1) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t start = 0;
    size_t n = data.length();
    if (eof)
        _eof_index = index + n;
    if (_index > index) {
        start = max(start, _index - index);
        if (start >= n) {
            if (_eof_index == _index) {
                _output.end_input();
            }
            return;
        }
    }
    n -= start;
    if (index + start == _index) {
        n = min(n, available_output_cap());
        _index += n;
        _output.write(data.substr(start, n));
        for (auto it = _buf.begin(); it != _buf.end() && it->first <= _index; it = _buf.erase(it)) {
            if (it->first == _index) {
                _output.write(std::string(1, it->second));
                _index++;
            }
        }
    } else {
        n = min(n, _capacity - _output.buffer_size() - (index + start) + _index);
        for (size_t i = start; i < n + start; i++) {
            _buf.insert(std::pair<size_t, char>(index + i, data[i]));
        }
    }
    if (_eof_index == _index) {
        _output.end_input();
    }
}

size_t StreamReassembler::available_output_cap() const { return _capacity - _output.buffer_size() - _buf.size(); }

size_t StreamReassembler::unassembled_bytes() const { return _buf.size(); }

bool StreamReassembler::empty() const { return _buf.empty(); }

std::optional<size_t> StreamReassembler::get_checkpoint() const {
    if (_index == 0)
        return std::nullopt;
    return std::make_optional(_index - 1);
}
