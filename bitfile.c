#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bitfile.h"

/*
*	Opens a bitfile from the file "name" in the mode given
*	modes: r,w,w+
*/
struct bitfile * bitfile_open(char * name, char *mode){
	FILE *fp;
	struct bitfile *result = malloc(sizeof(struct bitfile));
	result->file = fopen(name, mode);				//open specified file in specified mode
	result->buffer = 0;						//initialise empty buffer
	result->index = 0;
	return result;
}

/*
*	returns 8 bits from the next unread part of bitfile as an int
*/
int read_bitfile(struct bitfile * bfile){
	int result;
	
	if(bfile -> index == 8 || (bfile -> index == 0 && bfile -> buffer == 0)){	//if buffer fully read or start of file

		bfile->buffer = fgetc(bfile->file);			//read next byte into buffer (moves one byte further in file)
		if(feof(bfile->file)){
			return -1;				
		}
		bfile->index = 0;
	}
	
	result = (bfile -> buffer << (bfile -> index)) & 0x80;//buffer lshifted by index & ANDed with 0x80(10000000) to get leftmost bit
	bfile-> index++;	
	return result;							//return leftmost bit
}

/*
*	writes character code as bits into file
*/
void bitfile_write(char *code, struct bitfile* bfile){
	int length_remaining = strlen(code);			
	int count = 0;
	while(length_remaining > 0){
		if(bfile->buffer > 127){				//if Buffer >= [1xxx xxxx] then overflow has occurred
			printf("%s\n", "OVERFLOW");
		}

		bfile->buffer = bfile->buffer << 1;			//left shift buffer by one
 		int mask = (code[count++] - '0');			//mask = code[pos] - 0x30 to convert char to bit
		bfile->buffer = bfile->buffer | mask;			//Buffer = [xxxx xxxx] OR [0000 000y];	(y = current bit of code)
		
		bfile->index++;
		if(bfile -> index > 7){
			fputc(bfile->buffer, bfile->file);		//write buffer byte to file, and reset buffer and index
			bfile->index = 0;
			bfile->buffer = 0;
		}
		length_remaining--;
	}
}

/*
*	empties any remaining bits in buffer, and indicates End Of Transmission
*/
void empty_buffer(struct bitfile * bfile, char *EOT_code){

	bitfile_write(EOT_code, bfile);					//write EOT to output

	if(bfile->index > 0){
		bfile->buffer = bfile->buffer << (8 - bfile->index);  //if buffer !empty lshift so remaining bits are most significant
		fputc(bfile->buffer , bfile->file);		      //Write buffer to output
	}
}

/*
*	Closes this bitfile
*/
char bitfile_close(struct bitfile * bfile){
	fclose(bfile -> file);
	free(bfile);
}
