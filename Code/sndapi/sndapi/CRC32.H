#ifndef _CRC32_H_
#define _CRC32_H_

extern unsigned long crc_table[256];

__inline unsigned long CRC_MEM(char *data, unsigned long size, unsigned long initial_crc = 0){
	if (!size) return 0;
	unsigned long crc = ~0;
	do {
		crc = (crc >> 8) ^ crc_table[(crc & 0xff) ^ *data];
		data++;
		size--;

	} while (size);
	crc ^= ~0;
	return crc;
}

__inline  unsigned long CRC_STRING(char *text){
	return CRC_MEM(text, (unsigned long)strlen(text));
}

#endif //_CRC32_H_