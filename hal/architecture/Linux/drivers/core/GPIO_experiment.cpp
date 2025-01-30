// GPIO sample program: sends data on GPIO 17, receives on GPIO 27.
// Public domain.

#include <linux/gpio.h> // everything about GPIOs
#include <sys/ioctl.h> // ioctl()
#include <fcntl.h> // open(), O_RDONLY
#include <unistd.h> // close()
#include <stdio.h> // printf()
#include <poll.h> // poll()
#include <pthread.h> // threading

// Chip 0 on older Pi models, chip 4 on Pi 5.
#define CHIP "/dev/gpiochip0"

// GPIO line numbers.
#define WRITE_GPIO 17
#define READ_GPIO 27

// Termination flag.
// This is not an example of good coding style.

volatile int TERM = 0;

// Data structures from Linux GPIO libs.
// These here are global because a thread needs them.
// This is not an example of good coding style.

struct gpioevent_request event_request;

struct pollfd poll_file_descriptor;


void *reader(void *arg)
{

	// Elevate reader thread priority and use a FIFO scheduler.

	pthread_t thread = pthread_self();
	const struct sched_param sparam = { .sched_priority = 99, };
	int res = pthread_setschedparam(thread, SCHED_FIFO, &sparam);
	if (res != 0) {
		printf("Failed to elevate reader thread priority!\n");
	}


	struct gpioevent_data event_data;

	poll_file_descriptor.fd = event_request.fd;
	poll_file_descriptor.events = POLLIN;

	// Some variables for evaluating throughput

	__u64 first_event_timestamp = 0;
	__u64 last_event_timestamp = 0;
	__u64 count_rising = 0;
	__u64 count_falling = 0;

	// Receive loop

	while (TERM == 0) {

		int poll_result = poll(&poll_file_descriptor, 1, 1); // time out after 1 milliseconds

		if (poll_result == 0) {
			// printf("Poll timeout.\n");
			continue;
		}

		if (poll_result < 0) {
			// printf("Poll error.\n");
			continue;
		}

		if (poll_result > 1) {
			// printf("Multiple events per poll.\n");
		}

		// The "revents" field counts returned events.
		// The "POLLIN" constant seems to be a bitmask.

		if (poll_file_descriptor.revents & POLLIN) {

			int read_result = read(poll_file_descriptor.fd, &event_data, sizeof(event_data));

			if (read_result == -1) {
				// printf("Read error.\n");
				continue;
			}

			if (event_data.id == GPIOEVENT_EVENT_RISING_EDGE) {
				count_rising++;
				// printf("Rising edge at %llu.\n", event_data.timestamp);
			} else if (event_data.id == GPIOEVENT_EVENT_FALLING_EDGE) {
				count_falling++;
				// printf("Falling edge at %llu.\n",event_data.timestamp);
			} else {
				// printf("Some other event?\n");
			}

			if (first_event_timestamp == 0) {
				first_event_timestamp = event_data.timestamp;
			} else {
				last_event_timestamp = event_data.timestamp;
			}
		}
	}

	printf("Received %llu rising and %llu falling edges.\n",count_rising,count_falling);

	__u64 duration = last_event_timestamp - first_event_timestamp;
	double seconds = ((double) duration / (double) 1000000000);

	printf("Total duration %llu ns (%f s).\n",duration,seconds);

	__u64 nanos_per_edge = duration / (count_rising + count_falling);

	printf("Average %llu ns (%llu microseconds) per edge.\n",nanos_per_edge,(nanos_per_edge/1000));

	__u64 per_second = count_rising / seconds;

	printf("Rising edge frequency %llu Hz.\n",per_second);

	close(poll_file_descriptor.fd);

	return 0;
}


int main(int argc, char * const *argv)
{

	int res; // various call results

	// Elevate main thread priority and use a FIFO scheduler.

	const struct sched_param sparam = { .sched_priority = 99, };
	res = sched_setscheduler(0, SCHED_FIFO, &sparam);
	if (res != 0) {
		printf("Failed to elevate main thread priority!\n");
	}

	// Data structures from Linux GPIO libs.

	struct gpiochip_info chip_info;
	struct gpiohandle_request handle_request;


	int file_descriptor = open(CHIP, O_RDONLY);

	if (file_descriptor < 0) {
		printf("Failed opening GPIO chip.\n");
		return 1;
	}

	res = ioctl(file_descriptor, GPIO_GET_CHIPINFO_IOCTL, &chip_info);

	if (res < 0) {
		printf("Failed getting chip information.\n");
		close(file_descriptor);
		return 1;
	}


	printf("GPIO chip information:\n");
	printf("name: %s\n",chip_info.name);
	printf("label: %s\n",chip_info.label);
	printf("lines: %i\n", chip_info.lines);

	for (int i = 0; i < chip_info.lines; i++) {

		struct gpioline_info line_info;

		line_info.line_offset = i;

		if (ioctl(file_descriptor, GPIO_GET_LINEINFO_IOCTL, &line_info) < 0) {
			printf("Failed getting line %i info.\n", i);
		} else {
			printf("%d %s\n",i,line_info.name);
		}
	}


	// Request events on the reading line.

	event_request.lineoffset = READ_GPIO;
	event_request.eventflags = GPIOEVENT_REQUEST_BOTH_EDGES;
	event_request.handleflags = GPIOHANDLE_REQUEST_INPUT | GPIOHANDLE_REQUEST_BIAS_PULL_DOWN;

	res = ioctl(file_descriptor, GPIO_GET_LINEEVENT_IOCTL, &event_request);

	if (res < 0) {
		printf("Failed requesting events.\n");
		close(file_descriptor);
		return 1;
	}


	// Request handle on writing line. Many can be requested instead of one.

	handle_request.lineoffsets[0] = WRITE_GPIO;
	handle_request.flags = GPIOHANDLE_REQUEST_OUTPUT | GPIOHANDLE_REQUEST_BIAS_PULL_DOWN;
	handle_request.lines = 1;

	res = ioctl(file_descriptor, GPIO_GET_LINEHANDLE_IOCTL, &handle_request);

	if (res < 0) {
		printf("Failed requesting write handle.\n");
		close(file_descriptor);
		return 1;
	}

	// Start a reader thread
	pthread_t reader_thread;
	pthread_create(&reader_thread, NULL, &reader, NULL);


	// Data handle for writing
	struct gpiohandle_data hdata;

	// Time variables for sleeping a short interval.
	struct timespec ts, tr;
	ts.tv_sec = 0;

	printf("------------ now writing and reading -------------------\n");

	// Write something out ad hope the reader reads it
	for (int i = 0; i < 1000; i++) {

		hdata.values[0] = 1;
		res = ioctl(handle_request.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &hdata);
		if (res == -1) {
			printf("Failed setting line value.\n");
		}

		// Try to sleep for 5000 nanoseconds.
		// In reality, due to call latencies, you end up sleeping longer.

		ts.tv_nsec = 5000;
		nanosleep(&ts, &tr);

		hdata.values[0] = 0;
		res = ioctl(handle_request.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &hdata);
		if (res == -1) {
			printf("Failed setting line value.\n");
		}

		ts.tv_nsec = 5000;
		nanosleep(&ts, &tr);
	}

	close(handle_request.fd);

	// Tell the thread to finish
	TERM = 1;

	// Give it time
	sleep(1);

	// Close resources
	close(file_descriptor);

	// Rejoin main thread with finished thread
	pthread_join(reader_thread,NULL);

	return 0;
}
