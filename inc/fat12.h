/*
 * fat12.h
 *
 *  Created on: 28/nov/2015
 *      Author: chmod775
 */

#ifndef INC_FAT12_H_
#define INC_FAT12_H_

struct FAT12_str {
	/* General Volume Boot Record */
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sector_count;
	uint8_t number_of_fats;
	uint16_t max_root_dir_entries;
	uint16_t sectors_per_fat;
	uint32_t root_dir_sector;
	uint8_t fat_type[8];
	uint16_t signature;
	uint32_t fat_base; // What sector does the FATs start at
	uint32_t data_start_addr; // What address does the data regions	start at
};


struct FAT12_dir_str {
	char filename[8];
	char extension[3];
	uint8_t attributes;
	uint8_t spare_1;
	uint8_t time_fine;
	uint16_t create_time;
	uint16_t create_date;
	uint16_t last_access_date;
	uint16_t EA_index;
	uint16_t last_modified_time;
	uint16_t last_modified_date;
	uint16_t first_cluster;
	uint32_t size;
};

struct FAT12_stream_str {
	uint16_t id;
	uint8_t attributes;
	bool eof;

	uint32_t byte_in_sector; // Point to the byte into the current sector
	uint32_t sector_in_cluster; // Point to the sector into the current cluster
	uint32_t cluster_in_file; // Point to the cluster into the file

	uint16_t actual_cluster;
	uint16_t next_cluster;
	uint32_t size;
};

typedef struct FAT12_stream_str FAT12_stream;

extern struct FAT12_str FAT12_Info;
extern struct FAT12_dir_str FAT12_dir;

int FAT12_init();
FAT12_stream FAT12_root_fopen(const char *filename);
bool FAT12_root_fexist(const char *filename);

void FAT12_fread(FAT12_stream *stream, uint8_t *data, uint32_t length);
void FAT12_fread_line(FAT12_stream *stream, uint8_t *data);

#endif /* INC_FAT12_H_ */
