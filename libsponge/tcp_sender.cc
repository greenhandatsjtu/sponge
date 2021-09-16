#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _timer(Timer()) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    bool fin;
    // When filling window, treat a '0' window size as equal to '1' but don't back off RTO
    if (_win == 0 && _bytes_in_flight == 0) {
        if (!_timer.running()) {
            _timer.start(_ms_current, _initial_retransmission_timeout << _n_consecutive_retx);
        }
        auto payload = stream_in().read(1);
        fin = stream_in().eof();
        TCPSegment segment = TCPSegment();
        segment.header().seqno = next_seqno();
        segment.payload() = Buffer(payload.c_str());
        if (payload.empty())
            segment.header().fin = fin;
        _next_seqno += segment.length_in_sequence_space();
        segments_out().push(segment);
        _outstanding_segments.push(segment);
        _bytes_in_flight += segment.length_in_sequence_space();
        return;
    }
    while (_next_seqno == 0 ||
           ((!stream_in().buffer_empty() || (stream_in().eof() && stream_in().bytes_read() + 2 != _next_seqno)) &&
            _win > _bytes_in_flight)) {
        if (!_timer.running()) {
            _timer.start(_ms_current, _initial_retransmission_timeout << _n_consecutive_retx);
        }
        bool syn = _next_seqno == 0;
        auto payload = stream_in().read(min(TCPConfig::MAX_PAYLOAD_SIZE, static_cast<size_t>(_win - _bytes_in_flight)));
        fin = stream_in().eof();
        TCPSegment segment = TCPSegment();
        segment.header().syn = syn;
        segment.header().seqno = next_seqno();
        segment.payload() = Buffer(payload.c_str());
        // Don't add FIN if this would make the segment exceed the receiver's window
        if (_win - _bytes_in_flight - segment.length_in_sequence_space() > 0) {
            segment.header().fin = fin;
        }
        _next_seqno += segment.length_in_sequence_space();
        _bytes_in_flight += segment.length_in_sequence_space();
        segments_out().push(segment);
        _outstanding_segments.push(segment);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    _win = window_size;
    bool flag = false;
    while (!_outstanding_segments.empty() && ackno > _outstanding_segments.front().header().seqno +
                                                         _outstanding_segments.front().length_in_sequence_space() - 1) {
        flag = true;
        _bytes_in_flight -= _outstanding_segments.front().length_in_sequence_space();
        _outstanding_segments.pop();
    }
    if (flag) {
        _n_consecutive_retx = 0;
        if (_bytes_in_flight != 0)
            _timer.start(_ms_current, _initial_retransmission_timeout);
    }
    if (_bytes_in_flight == 0)
        _timer.stop();
    if (_win > 0)
        fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _ms_current += ms_since_last_tick;
    if (_timer.expired(_ms_current) && !_outstanding_segments.empty()) {
        _segments_out.push(_outstanding_segments.front());
        if (_win != 0) {
            ++_n_consecutive_retx;
        }
        _timer.start(_ms_current, _initial_retransmission_timeout << _n_consecutive_retx);
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _n_consecutive_retx; }

void TCPSender::send_empty_segment() {
    TCPSegment segment = TCPSegment();
    segment.header().seqno = wrap(_next_seqno, _isn);
    segment.payload() = Buffer();
    _segments_out.push(segment);
}

void Timer::start(const size_t ms_start, const size_t retx_timeout) {
    _ms_start = ms_start;
    _retx_timeout = retx_timeout;
    _start = true;
}

bool Timer::expired(const size_t _ms_current) const { return _start && _ms_current - _ms_start >= _retx_timeout; }
