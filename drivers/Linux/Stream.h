#ifndef Stream_h
#define Stream_h

class Stream {
	public:
		virtual size_t write(uint8_t) = 0;
		virtual size_t write(const uint8_t *buffer, size_t size) = 0;

		virtual int available() = 0;
		virtual int read() = 0;
		virtual int peek() = 0;
		virtual void flush() = 0;
};

#endif
