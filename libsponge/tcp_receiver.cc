#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

using namespace std;

TCPReceiver::TCPReceiver(const size_t capacity)
    : _reassembler(capacity), _capacity(capacity), _isn(WrappingInt32{0}), _syn_received(false), _fin_received(false) {}

void TCPReceiver::segment_received(const TCPSegment &seg) {
    auto header = seg.header();
    if (header.syn) {
        _syn_received = true;
        _isn = header.seqno;
    } else if (!_syn_received) {
        return;
    }
    if (header.fin) {
        _fin_received = true;
    }
    _reassembler.push_substring(
        seg.payload().copy(),
        unwrap(header.syn ? header.seqno : header.seqno - 1, _isn, _reassembler.get_checkpoint().value_or(0)),
        header.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_syn_received)
        return std::nullopt;
    uint64_t checkpoint;
    auto c = _reassembler.get_checkpoint();
    if (c.has_value()) {
        checkpoint = c.value() + 2;
    } else {
        checkpoint = 1;
    }
    if (_fin_received && _reassembler.stream_out().input_ended()) {
        checkpoint += 1;
    }
    return std::make_optional(wrap(checkpoint, _isn));
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
