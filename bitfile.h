//Allows access to individual bits in a file
struct bitfile{
	FILE *file;
	unsigned char buffer;
	int index;
};

/*
*	Opens a bitfile from the file "name" in the mode given
*	modes: r,w,w+
*/
struct bitfile * bitfile_open(char *name, char *mode);

/*
*	returns 8 bits from the next unread part of bitfile as an int
*/
int read_bitfile(struct bitfile * bfile);

/*
*	writes character code as bits into file
*/
void bitfile_write(char *code, struct bitfile *bfile);

/*
*	empties any remaining bits in buffer, and indicates End Of Transmission
*/
void empty_buffer(struct bitfile * bfile, char *EOT_code);

/*
*	Closes this bitfile
*/
char bitfile_close(struct bitfile * bfile);
