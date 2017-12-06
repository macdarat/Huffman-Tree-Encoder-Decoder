#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "bitfile.h"
#define NUM_CHARS 255

//represents a compound character node
struct compound{
	struct huffnode *left;
	struct huffnode *right;
};
//A node in the huffman tree
struct huffnode{
	int freq;
	int is_leaf;		//boolean
	unsigned char chr;
	struct compound cmp;
};

/*
* Removes node with the smallest frequency from the list, and returns it
* If two nodes have the same frequency, the node with the larger ASCII value is chosen
*/
struct huffnode *remove_smallest(struct huffnode **list, int size){
	int smallest_i = 0;
	
	for(int i=0; i<size; i++){					//loop through list
		if(list[i]->freq < list[smallest_i]->freq){		//if smaller node found
			smallest_i = i;
		}
		else if(list[i]->freq == list[smallest_i]->freq){	//if two smallest have same frequency
			if(list[i]->chr > list[smallest_i]->chr){
				smallest_i = i;
			}
		}
	}
	struct huffnode *result = list[smallest_i];
	list[smallest_i] = list[size-1];			//replace smallest by last in list, then shorten list by one
	
	list[size-1] = NULL;
	return result;
}

/*
* Builds the huffman tree from the list by removing the two smallest nodes, and combining them into a compound node 
* with freq = freq1+freq2. Returns the remaining size of the list
*/
int build_tree(struct huffnode **list, int *freqs, int no_freqs){
	int size = no_freqs;
	struct huffnode *smallest, *second_smallest, *compound_node;
	//get two smallest nodes
	smallest = remove_smallest(list, size);
	size--;
	second_smallest = remove_smallest(list, size);
	size--;
	
	//combine into compound
	compound_node = malloc(sizeof(struct huffnode));
	compound_node->freq = smallest->freq + second_smallest->freq;
	compound_node->is_leaf = 0;
	compound_node->cmp.left = smallest;
	compound_node->cmp.right = second_smallest;
	compound_node->chr = second_smallest->chr;
	size++;
	list[size-1] = compound_node;
	
	return size;
}

/*
* Recursively walks the huffman tree to generate the encodings. Encodings are stored in the passed char**array as strings
*  of '1's and '0's. Returns 1 if node is a leaf, 0 otherwise
*/
int walk_tree(struct huffnode *node, char* code, char **table){	
	//if leaf store current code at table[char value]
	if(node->is_leaf == 1){
		int index = (int)(node->chr);
		char* copy=malloc(sizeof(char) * 64);
		strcpy(copy, code);
		
		table[index] = copy;
		return 1;
	}
	//else explore left and right subtrees. 1 is concatnated to current code for right, 0 for left.
	// => Every level down has a larger code 
	else{
		//go left
		char* prev = malloc(sizeof(char) * 64);
		strcpy(prev, code);
		char *new_code = strcat(prev, "0");
		int result = walk_tree(node->cmp.left, new_code, table);

		//go right
		strcpy(prev, code);
		new_code =strcat(prev, "1");
		result = walk_tree(node->cmp.right, new_code, table);
		return 0;
	}
}

/*
* Creates the table of encodings by using the recursive method walk_tree starting from the root of the tree
* Returns char** array where array[index] gives the string encoding of the character with value = index
*/
char **create_table(struct huffnode **list){
	char **table = malloc(sizeof(char*) * NUM_CHARS);
	int c = walk_tree(list[0], "", table);
	
	return table;
}

/*
* Creates a file containing the encodings
*/
void create_output_file(char **table){
	FILE *output;
	output = fopen("/home/mininet/Desktop/C/lab6/encoding.txt", "w+");
	
	//if not nullpointer, output encoding for char on line number char
	if(table){						
		for(int i = 0; i<NUM_CHARS; i++){
			fputs(table[i], output);
			fputs("\n", output);
		}
	}
	fputc(0x04, output);	//EOT
	fclose(output);
}

/*
* Builds the list of simple leaf nodes from the array of frequencies. frequencies[character value] gives the freq of the character
*/
void build_list(struct huffnode**list, int *frequencies){
	for(int i = 0; i<NUM_CHARS; i++){
		//ensure all nodes have a frequency
		if( frequencies[i] == 0){
			frequencies[i] = 1;
		}
		list[i] = malloc(sizeof(struct huffnode));
		list[i]->freq = frequencies[i];
		list[i]->is_leaf = 1;
		list[i]->chr = i;
	}
	free(frequencies);
}

/*
* Reads one character from the encoded file by navigating the tree according to which bit is currently read from the bitfile
*/
char read_char(struct huffnode *root, struct bitfile *file){
	int bit;
	struct huffnode *pt;
	pt = root;
	//while code hasn't navigated to a leaf 
	while(!pt->is_leaf){
		bit = read_bitfile(file);
		if(bit == 0){
			pt = pt->cmp.left;
		}
		else {
			pt = pt->cmp.right;
		}
	}
	return pt->chr;
}

/*
* Creates the encoded compressed file from the array of encodings one character at a time
*/
void encode(char **encoding, char *filename, char *output){	
	struct bitfile *bfile = bitfile_open(output, "w+");
	unsigned char chr;
	FILE *input;
	input = fopen(filename, "r");
	if(input == NULL){
		printf("%s\n", "ERROR");
	}
	
	//get char data from file and write it in encoded compressed form using table of encodings
	chr = fgetc(input);
	while(!feof(input)){
		char *code = encoding[(int)chr];
		bitfile_write(code, bfile);
		chr = fgetc(input);
	}
	
	//ensure all chars written and close file
	empty_buffer(bfile, encoding[4]);
	bitfile_close(bfile);
}

/*
* Creates a decoded result from the encoded compressed file passed, and stores it in a file named by output
*/
void decode(struct huffnode *root, char *filename, char *output){
	struct bitfile *bfile = bitfile_open(filename, "r");
	FILE *result;
	result = fopen(output, "w+");
	int count = 0;
	
	//reads chars from bitfile and adds them to output file until EOT
	char decoded_char = read_char(root, bfile);
	while(decoded_char != 0x04 && decoded_char != 0x255){		//while not EOT and not error
		fputc(decoded_char, result);
		decoded_char = read_char(root, bfile);
		printf("%c", decoded_char);
		count++;
	}
	bitfile_close(bfile);
}

/************************************************************************************************
*  					MAIN							*
* Either encodes or decodes a passed file, using a training file to build the huffman tree. 	*
* Also outputs a file containing encodings used							*
*************************************************************************************************/
int main(int argc, char**argv){
	unsigned char c;
	FILE *file;
	struct huffnode **list = malloc(sizeof(struct huffnode *) * NUM_CHARS);
	
  	if (argc != 5){
    	fprintf(stderr, "Usage: huffman <operation> <input filename> <training filename> <output filename>\n");
    	exit(1);	    // exit with error code
  	}

	file = fopen(argv[3], "r");
	assert(file != NULL);
	int *frequencies = malloc(sizeof(int) * NUM_CHARS);
	
	c = fgetc(file);	// attempt to read a byte
	while(!feof(file)) {
		frequencies[(int)c] += 1;
		c = fgetc(file);
	}
	
	//build huffman tree by building list of simple chars then converting to tree
	build_list(list, frequencies);
	int size = NUM_CHARS;
	while(size>1){				//while still a list
		size = build_tree(list, frequencies, size);
	}
	
	char **encoding = create_table(list);
	for(int i =0; i<NUM_CHARS; i++){
		printf("%c",i);
		printf("%s", " encoded to:");
		printf("%s\n",encoding[i]);
	}
	create_output_file(encoding);
	
	if(strcmp(argv[1], "encode") == 0){
		encode(encoding, argv[2], argv[4]);
	}
	else if (strcmp(argv[1], "decode") == 0){
		decode(list[0], argv[2], argv[4]);
	}
	else{
		fprintf(stderr, "Parameter argv[1] is not a valid operator. Use <encode> or <decode>\n");
    	exit(1);	    // exit with error code
	}	
	fclose(file);
	return 0;  // exit without error code
}
