#include "hal/transport/CC1101/driver/CC1101.h"

#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
#include "drivers/CircularBuffer/CircularBuffer.h"

typedef struct _transportQueuedMessage {
	uint8_t m_len;                        // Length of the data
	uint8_t m_data[MAX_MESSAGE_SIZE];   // The raw data
} transportQueuedMessage;

/** Buffer to store queued messages in. */
static transportQueuedMessage transportRxQueueStorage[MY_RX_MESSAGE_BUFFER_SIZE];
/** Circular buffer, which uses the transportRxQueueStorage and administers stored messages. */
static CircularBuffer<transportQueuedMessage> transportRxQueue(transportRxQueueStorage,
        MY_RX_MESSAGE_BUFFER_SIZE);

static volatile uint8_t transportLostMessageCount = 0;

static void transportRxCallback(void)
{
	// Called for each message received by radio, from interrupt context.
	// This function _must_ call CC1101_readMessage() to de-assert interrupt line!
	if (!transportRxQueue.full()) {
		transportQueuedMessage* msg = transportRxQueue.getFront();
		msg->m_len = CC1101_readMessage(msg->m_data);		// Read payload & clear RX_DR
		(void)transportRxQueue.pushFront(msg);
	} else {
		// Queue is full. Discard message.
		(void)CC1101_readMessage(NULL);		// Read payload & clear RX_DR
		// Keep track of messages lost. Max 255, prevent wrapping.
		if (transportLostMessageCount < 255) {
			++transportLostMessageCount;
		}
	}
}
#endif

bool transportInit(void)
{
#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
	CC1101_registerReceiveCallback( transportRxCallback );
#endif
	const bool result = CC1101_initialise();
#if !defined(MY_GATEWAY_FEATURE) && !defined(MY_CC1101_ATC_MODE_DISABLED)
	CC1101_setATC(true, CC1101_TARGET_RSSI);
#endif
	return result;
}

void transportSetAddress(const uint8_t address)
{
	CC1101_setAddress(address);
}

uint8_t transportGetAddress(void)
{
	return CC1101_getAddress();
}

bool transportSend(const uint8_t to, const void *data, const uint8_t len, const bool noACK)
{
	return CC1101_sendWithRetry(to, data, len, noACK);
	//	return CC1101_sendWithRetry(to, data, len, 1);
}

bool transportDataAvailable(void)
{
#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
	(void)CC1101_isDataAvailable;				// Prevent 'defined but not used' warning
	return !transportRxQueue.empty();
#else
	CC1101_handle();
	return CC1101_packetAvailable();
#endif
}

bool transportSanityCheck(void)
{
	return CC1101_sanityCheck();
}

uint8_t transportReceive(void *data)
{
	uint8_t len = 0;
#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
	transportQueuedMessage* msg = transportRxQueue.getBack();
	if (msg) {
		len = msg->m_len;
		(void)memcpy(data, msg->m_data, len);
		(void)transportRxQueue.popBack();
	}
#else
	len = CC1101_getData(static_cast<uint8_t *>(data), MAX_MESSAGE_SIZE);
#endif
	return len;
}

void transportSleep(void)
{
	CC1101_sleep();
}

void transportStandBy(void)
{
	CC1101_wakeUp();
}

void transportPowerDown(void)
{
	// No power control.
}

void transportPowerUp(void)
{
	// No power control.
}

int16_t transportGetSendingRSSI(void)
{
	return CC1101_getSendingRSSI();
}

int16_t transportGetReceivingRSSI(void)
{
	return CC1101_getReceivingRSSI();
}

int16_t transportGetSendingSNR(void)
{
	return INVALID_SNR;
}

int16_t transportGetReceivingSNR(void)
{
	return INVALID_SNR;
}

int16_t transportGetTxPowerPercent(void)
{
	return static_cast<int16_t>(CC1101_getTxPowerPercent());
}

int16_t transportGetTxPowerLevel(void)
{
	return static_cast<int16_t>(CC1101_getTxPowerLevel());
}

bool transportSetTxPowerPercent(const uint8_t powerPercent)
{
	return CC1101_setTxPowerPercent(powerPercent);
}
