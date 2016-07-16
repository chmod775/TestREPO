/*
 * fat12.c
 *
 *  Created on: 28/nov/2015
 *      Author: chmod775
 */
#include <stdlib.h>
#include "board.h"
#include "fat12.h"
#include "flash.h"

#include <string.h>

struct FAT12_str FAT12_Info;
struct FAT12_dir_str FAT12_dir;

uint32_t P1_LBA;

uint8_t vBlock[0x210];

void FAT12_read_block(uint8_t *block_data, uint32_t block_address) {
	FLASH_read(0x200 * block_address, 0x200, block_data);
}

unsigned int FAT12_partition1_LBA() {
	FAT12_read_block(vBlock, 0);

	if ((vBlock[0] == 0xeb) && (vBlock[1] == 0x3c) && (vBlock[2] == 0x90)) return 0; // No MBR!

    unsigned int addr = vBlock[454] | (vBlock[455] << 8) | (vBlock[456] << 16) | (vBlock[457] << 24);
    return addr;
}

void FAT12_read_info() {
	FAT12_read_block(vBlock, P1_LBA);

	FAT12_Info.bytes_per_sector = vBlock[0x0b] | (vBlock[0x0c] << 8);
	FAT12_Info.sectors_per_cluster = vBlock[0x0d];
	FAT12_Info.reserved_sector_count = vBlock[0x0e] | (vBlock[0x0f] << 8);
	FAT12_Info.number_of_fats = vBlock[0x10];
	FAT12_Info.max_root_dir_entries = vBlock[0x11] | (vBlock[0x12] << 8);
	FAT12_Info.sectors_per_fat = vBlock[0x16] | (vBlock[0x17] << 8);
	FAT12_Info.signature = vBlock[0x1fe] | (vBlock[0x1ff] << 8);

	/* At what sector does the fat table begin? */
	FAT12_Info.fat_base = P1_LBA + FAT12_Info.reserved_sector_count;
	/* At what sector does the root directory start at? */
	FAT12_Info.root_dir_sector = FAT12_Info.fat_base + FAT12_Info.sectors_per_fat*FAT12_Info.number_of_fats;
	/* At what address does the data region start at? */
	FAT12_Info.data_start_addr = FAT12_Info.root_dir_sector*FAT12_Info.bytes_per_sector + FAT12_Info.max_root_dir_entries*32;
}

int FAT12_init() {
    // Read the first partition LBA //
    P1_LBA = FAT12_partition1_LBA();

    // Read FAT infos //
    FAT12_read_info();

    return 0;
}

uint32_t FAT12_count_root_entries(uint8_t attributes_mask) {
	uint32_t n_blocks = FAT12_Info.max_root_dir_entries >> 5; // Divide by 16

	volatile uint32_t i, j;
	volatile uint32_t cnt = 0;

	bool dir_ended = false;

	for (i = 0; i < n_blocks; i++) {
		FAT12_read_block(vBlock, FAT12_Info.root_dir_sector + i);

		for (j = 0; j < 16; j++) {
			memcpy(&FAT12_dir, &vBlock[j * 32], 32);

			if (FAT12_dir.filename[0] == 0x00) dir_ended = true;

			if (FAT12_dir.attributes != 0x0f) {
				if (FAT12_dir.attributes & attributes_mask) cnt++;
			}

			if (dir_ended) break;
		}
		if (dir_ended) break;
	}

	return cnt;
}

uint16_t FAT12_fat_entry(uint16_t entry_index) {
	uint32_t entry_address = entry_index + (entry_index >> 1);
	uint32_t entry_sector = entry_address / FAT12_Info.bytes_per_sector;

	FAT12_read_block(vBlock, FAT12_Info.fat_base + entry_sector);

	bool entry_odd = entry_index & 0x01;
	uint16_t entry_data = 0;

	if (entry_odd) {
		entry_data = ((vBlock[entry_address] & 0xF0) >> 4) | (vBlock[entry_address + 1] << 4);
	} else {
		entry_data = vBlock[entry_address] | ((vBlock[entry_address + 1] & 0x0F) << 8);
	}

	return entry_data;
}


uint16_t FAT12_next_cluster(uint16_t actual_cluster) {

}


bool FAT12_filename_cmp(char *filename_flash, char *filename) {
	volatile uint32_t i;
	bool ret = true;

	char tmp_filename[8];

	memset(tmp_filename, 0x20, 8);
	memcpy(tmp_filename, filename, strlen(filename));

	for (i = 0; i < 8; i++) {
		if (filename_flash[i] != tmp_filename[i]) {
			ret = false;
			break;
		}
	}

	return ret;
}

FAT12_stream FAT12_root_fopen(const char *filename) {
	FAT12_stream ret_stream;

	uint32_t n_blocks = FAT12_Info.max_root_dir_entries >> 5; // Divide by 16

	volatile uint32_t i, j;
	volatile uint32_t id = 0;

	bool dir_ended = false;

	ret_stream.id = 0;
	ret_stream.actual_cluster = 0;
	ret_stream.byte_in_sector = 0;
	ret_stream.sector_in_cluster = 0;
	ret_stream.cluster_in_file = 0;
	ret_stream.eof = false;

	for (i = 0; i < n_blocks; i++) {
		FAT12_read_block(vBlock, FAT12_Info.root_dir_sector + i);

		for (j = 0; j < 16; j++) {
			memcpy(&FAT12_dir, &vBlock[j * 32], 32);

			if (FAT12_dir.filename[0] == 0x00) dir_ended = true;

			if (FAT12_dir.attributes != 0x0f) {
				//if (FAT12_dir.attributes == 0) {
					if (FAT12_filename_cmp(FAT12_dir.filename, filename)) {
						ret_stream.id = id;
						ret_stream.attributes = FAT12_dir.attributes;
						ret_stream.byte_in_sector = 0;
						ret_stream.sector_in_cluster = 0;
						ret_stream.actual_cluster = FAT12_dir.first_cluster;
						ret_stream.next_cluster = FAT12_fat_entry(ret_stream.actual_cluster);
						ret_stream.size = FAT12_dir.size;
						dir_ended = true;
					}
				//}
			}

			id++;

			if (dir_ended) break;
		}
		if (dir_ended) break;
	}

	return ret_stream;
}

bool FAT12_root_fexist(const char *filename) {
	FAT12_stream t_stream = FAT12_root_fopen(filename);

	return (t_stream.actual_cluster < 2) ? false : true;
}

void FAT12_fread(FAT12_stream *stream, uint8_t *data, uint32_t length) {
	volatile uint32_t i;

	FLASH_read(FAT12_Info.data_start_addr + ((stream->actual_cluster - 2) * FAT12_Info.sectors_per_cluster * FAT12_Info.bytes_per_sector) + (stream->sector_in_cluster * FAT12_Info.bytes_per_sector), FAT12_Info.bytes_per_sector, vBlock);

	for (i = 0; i < length; i++) {
		if (stream->byte_in_sector >= FAT12_Info.bytes_per_sector) {
			stream->byte_in_sector = 0;
			stream->sector_in_cluster++;

			FLASH_read(FAT12_Info.data_start_addr + ((stream->actual_cluster - 2) * FAT12_Info.sectors_per_cluster * FAT12_Info.bytes_per_sector) + (stream->sector_in_cluster * FAT12_Info.bytes_per_sector), FAT12_Info.bytes_per_sector, vBlock);
		}

		if (stream->sector_in_cluster >= FAT12_Info.sectors_per_cluster) {
			stream->sector_in_cluster = 0;

			if (stream->next_cluster < 0xff8) {
				stream->actual_cluster = stream->next_cluster;
				stream->next_cluster = FAT12_fat_entry(stream->actual_cluster);
			} else {
				stream->eof = true;
				break;
			}

			FLASH_read(FAT12_Info.data_start_addr + ((stream->actual_cluster - 2) * FAT12_Info.sectors_per_cluster * FAT12_Info.bytes_per_sector) + (stream->sector_in_cluster * FAT12_Info.bytes_per_sector), FAT12_Info.bytes_per_sector, vBlock);
		}

		*data = vBlock[stream->byte_in_sector];

		data++;
		stream->byte_in_sector++;
	}
}

void FAT12_fread_line(FAT12_stream *stream, uint8_t *data) {
	bool line_ended = false;

	FLASH_read(FAT12_Info.data_start_addr + ((stream->actual_cluster - 2) * FAT12_Info.sectors_per_cluster * FAT12_Info.bytes_per_sector) + (stream->sector_in_cluster * FAT12_Info.bytes_per_sector), FAT12_Info.bytes_per_sector, vBlock);

	do {
		if ((stream->cluster_in_file * FAT12_Info.sectors_per_cluster * FAT12_Info.bytes_per_sector) + (stream->sector_in_cluster * FAT12_Info.bytes_per_sector) + stream->byte_in_sector >= stream->size) {
			data++;
			*data = 0x00; // Terminate the string

			stream->eof = true;
			line_ended = true;
			break;
		}
		if (stream->byte_in_sector >= FAT12_Info.bytes_per_sector) {
			stream->byte_in_sector = 0;
			stream->sector_in_cluster++;

			FLASH_read(FAT12_Info.data_start_addr + ((stream->actual_cluster - 2) * FAT12_Info.sectors_per_cluster * FAT12_Info.bytes_per_sector) + (stream->sector_in_cluster * FAT12_Info.bytes_per_sector), FAT12_Info.bytes_per_sector, vBlock);
		}

		if (stream->sector_in_cluster >= FAT12_Info.sectors_per_cluster) {
			stream->sector_in_cluster = 0;
			stream->cluster_in_file++;

			if (stream->next_cluster < 0xff8) {
				stream->actual_cluster = stream->next_cluster;
				stream->next_cluster = FAT12_fat_entry(stream->actual_cluster);
			} else {
				stream->eof = true;
				line_ended = true;
				break;
			}

			FLASH_read(FAT12_Info.data_start_addr + ((stream->actual_cluster - 2) * FAT12_Info.sectors_per_cluster * FAT12_Info.bytes_per_sector) + (stream->sector_in_cluster * FAT12_Info.bytes_per_sector), FAT12_Info.bytes_per_sector, vBlock);
		}

		*data = vBlock[stream->byte_in_sector];
		if ((*data == 0x0a) || (*data == 0x0d)) {
			*data = 0x00; // Terminate the string

			line_ended = true;
		}

		data++;
		stream->byte_in_sector++;
	} while (!line_ended);
}
