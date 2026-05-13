#include "ring_buffer.h"
#include <stddef.h>

void RingBuffer_Init(RingBuffer_t *pRingBuffer, volatile uint8_t *pBuffer, uint32_t Size)
{
	if ((pRingBuffer == NULL) || (pBuffer == NULL) || (Size == 0U))
	{
		return;
	}

	pRingBuffer->buffer = pBuffer;
	pRingBuffer->head = 0;
	pRingBuffer->tail = 0;
	pRingBuffer->size = Size;
}

uint8_t RingBuffer_IsEmpty(RingBuffer_t *pRingBuffer)
{
	if (pRingBuffer == NULL)
	{
		return 1;
	}

	return (pRingBuffer->head == pRingBuffer->tail);
}

uint8_t RingBuffer_IsFull(RingBuffer_t *pRingBuffer)
{
	if ((pRingBuffer == NULL) || (pRingBuffer->size == 0U))
	{
		return 0;
	}

	uint32_t next_head = (pRingBuffer->head + 1U) % pRingBuffer->size;
	return (next_head == pRingBuffer->tail);
}

uint8_t RingBuffer_Write(RingBuffer_t *pRingBuffer, uint8_t Data)
{
	if (pRingBuffer == NULL)
	{
		return 0;
	}

	if (RingBuffer_IsFull(pRingBuffer))
	{
		return 0;
	}

	pRingBuffer->buffer[pRingBuffer->head] = Data;
	pRingBuffer->head = (pRingBuffer->head + 1U) % pRingBuffer->size;
	return 1;
}

uint8_t RingBuffer_Read(RingBuffer_t *pRingBuffer, uint8_t *pData)
{
	if ((pData == NULL) || RingBuffer_IsEmpty(pRingBuffer))
	{
		return 0;
	}

	*pData = pRingBuffer->buffer[pRingBuffer->tail];
	pRingBuffer->tail = (pRingBuffer->tail + 1U) % pRingBuffer->size;
	return 1;
}
