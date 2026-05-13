#ifndef INC_RING_BUFFER_H_
#define INC_RING_BUFFER_H_

#include <stdint.h>

typedef struct
{
	volatile uint8_t *buffer;
	volatile uint32_t head;
	volatile uint32_t tail;
	uint32_t size;
} RingBuffer_t;

void RingBuffer_Init(RingBuffer_t *pRingBuffer, volatile uint8_t *pBuffer, uint32_t Size);
uint8_t RingBuffer_IsEmpty(RingBuffer_t *pRingBuffer);
uint8_t RingBuffer_IsFull(RingBuffer_t *pRingBuffer);
uint8_t RingBuffer_Write(RingBuffer_t *pRingBuffer, uint8_t Data);
uint8_t RingBuffer_Read(RingBuffer_t *pRingBuffer, uint8_t *pData);

#endif /* INC_RING_BUFFER_H_ */
