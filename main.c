/* 
	Rombus Game Boy camera dumper

	This program extracts thumbnails & full size photos from game boy 
	camera saves.
	 
	Author: Iber Parodi Siri <iberparodisiri@gmail.com>
	http://rombusevilbones.blogspot.com/
	Created on 23/05/2013, 20:45

	 
	This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Size in tiles of full size pic and thumbnail
#define WIDTH 16
#define HEIGHT 14
#define WIDTH_THUMB 4
#define HEIGHT_THUMB 4

#define PIXELES 8	//Cant of pixels per row and column
#define CANT_PICS 30

typedef unsigned char c, byte;
void extractToBuff(FILE *input, int **bmpBuff, int altoTile, int anchoTile); // It needs height and width for extracting thumbnails or full size
void printToFile(FILE *output,int **bmpBuff, int altoTile, int anchoTile);
int isBlank(int **bmpBuff, int altoTile, int anchoTile);
byte *createHeader(char thumbnailFlag);
void printHelp();

int main(int argc, char **argv) {
	FILE *input;
	char thumbnailflag = 0;
	char name[20];			// Name without path nor extension	
	int step;				// Distance between photos
	int height, width;

// Validate Arguments
	if(!(argc-1) || !strcmp("--help",argv[1]) || !strcmp("-h",argv[1])){
		printHelp();
		exit(0);
	}	
	input = fopen(argv[1],"rb");
	if(!input){			
		printHelp();
		printf("\nSTATUS:\tError opening input file.\n\n");
		exit(0);
	}
	if(argc > 2){
		if(!strcmp("--thumbnail",argv[2]))
			thumbnailflag = 1;
		else 
			strcpy(name,argv[2]);
	}
	else{
		printHelp();
		printf("\nSTATUS:\tOutput name missing.\n\n");
		exit(0);
	}

	
// Set's vars acording to photo size
	if(!thumbnailflag){
		fseek(input,0x2000,SEEK_SET); // Seek 1º photo
		step = 0x200;
		height = HEIGHT;
		width = WIDTH;
	}
	else{
		fseek(input,0x2e00,SEEK_SET); // Seek 1º photo
		step = 0xF00;
		height = HEIGHT_THUMB;
		width = WIDTH_THUMB;
		strcpy(name,"thumb_");
	}
	
// Extracts photos
	byte *header = createHeader(thumbnailflag);	
	int **bmpBuff = (int **)malloc(sizeof(int*) * (height*PIXELES));	// This stores the pic data before it get's printed to the file
	for(int i=0; i< height*PIXELES; bmpBuff[i++] =  (int *)malloc(sizeof(int) * (width*PIXELES)));

	for(int i=0; i< CANT_PICS; i++, fseek(input,step,SEEK_CUR)){
		int blank = isBlank(bmpBuff,height,width);
		extractToBuff(input,bmpBuff,height,width);
		
		if(!blank){
			FILE *output;
			char tmpName[30], index[2];
			
			sprintf(index, "%d", i+1);
			strcat(strcat(strcpy(tmpName,name),index),".bmp");
			output = fopen(tmpName,"wb");
			
			fwrite(header,70,1,output);
			printToFile(output,bmpBuff,height,width);
			
			fclose(output);
			printf("%s dumped.\n",tmpName);
		}
//		else break; // I'm not sure if the first blank means that from that point on all will be blank
	}
	
// Freeing 'n closing	
	for(int i=0; i<height*PIXELES; free(bmpBuff[i++]));	
	free(bmpBuff);
	free(header);
	fclose(input);
	
	printf("Finished.\n");
	return (EXIT_SUCCESS);	
}

void extractToBuff(FILE *input, int **bmpBuff, int altoTile, int anchoTile){
	int ancho, alto, inAncho, inAlto, y, x;	
	char buff[2];
	
	for(alto=0; alto < altoTile; alto++)								// Width in tiles	
		for(ancho=0; ancho < anchoTile; ancho++)						// Height en tiles
			for(inAlto=0; inAlto < PIXELES; inAlto++){					// Each pixel coordinate Y
				fread(buff,2,1,input);	// Reads 8px that compose the tile row
				int z = 128;			// Bit mask 1000 0000
				
				for(inAncho=0; inAncho < PIXELES ; inAncho++, z>>=1){	// Each pixel coordinate X
					int color = ((*buff & z) == z) * 2 + ((*(buff + 1) & z) == z);
					x = inAncho+(ancho*PIXELES);
					y = inAlto+(alto*PIXELES);
					
					bmpBuff[y][x] = color;	// Writes the pixel into the buffer
				}
			}
}

int isBlank(int **bmpBuff, int altoTile, int anchoTile){
	int col = (altoTile == 4)? 27:(altoTile*PIXELES)-1;	// Set height according to thumbnail or full size
	int blank=1;
	
	for(int y = col; y >= 0 && blank; y--){
		for(int x = 0; x < anchoTile*PIXELES; x+=2){			
			if(blank && (bmpBuff[y][x] || bmpBuff[y][x+1])){
				blank = 0;	//not a blank
				break;
			}
		}
	}
	return blank;
}

void printToFile(FILE *output,int **bmpBuff, int altoTile, int anchoTile){
	int colors[] = {3,1,2,0};
	int col = (altoTile == 4)? 27:(altoTile*PIXELES)-1;	// Set height according to thumbnail or full size
	
	for(int y = col; y >= 0; y--)
		for(int x = 0; x < anchoTile*PIXELES; x+=2){
			int byte = colors[bmpBuff[y][x]] << 4 | colors[bmpBuff[y][x+1]];	// Make 1 byte from 2px
			fputc(byte,output);
		}
}

byte *createHeader(char thumbnailFlag){
	int headerSize = 70;
	byte *header=(byte *)malloc(headerSize);
	for(int i=0; i<headerSize; i++){
		switch(i){
			case 0:		// Signature | 2 bytes
				header[i] = 0x42; break;
			case 1:
				header[i] = 0x4D; break;				
			case 10:	// Offset to bitmap data | 4 bytes | 70 = header + 4 colors
				header[i] = 0x46; break;
			case 14:	// Size of info header | 4 bytes | 40 Bytes = 0x28
				header[i] = 0x28; break;
			case 18:	// Height in pixels | 4 bytes | Depens on thumbnail or big photo
				if(!thumbnailFlag)
					header[i] = 0x80;  // 128
				else
					header[i] = 0x20;  // 32
				break;
			case 22:	// Width in pixels | 4 bytes | Depens on thumbnail or big photo
				if(!thumbnailFlag)
					header[i] = 0x70;  // 112
				else			
					header[i] = 0x1C;  // 28
				break;
			case 26:	// Color planes | 2 bytes
				header[i] = 0x01; break;
			case 28:	// Bits per pixel | 2 bytes
				header[i] = 0x04; break;
			case 46:	// Number of colors in palete | 4 bytes
				header[i] = 0x04; break;
			// ---COLOR PALETTE --- | 4 bytes per color (one reserved)
			case 54: case 55: case 56:	//Dark
				header[i]=0x00;	break;
			case 58: case 59:case 60:	//Light Dark
				header[i] = 128; break;
			case 62: case 63: case 64:	//Dark bright
				header[i] = 192; break;
			case 66: case 67: case 68:	//Bright
				header[i] = 0xFF; break;
			//-------------------------//
			default:
				header[i] = 0x00; break;
		}
	}
	return header;
}

void printHelp(){
	printf( "\nRombus Game Boy camera dumper \n\n"
			"Usage: ./rgbcdumper <input.sav>  < --thumbnail  |  outputname >\n\n"
			"Written by Iber Parodi Siri <iberparodisiri@gmail.com>\n"
			"See web site for more info:\n"
			"	http://rombusevilbones.blogspot.com/"
			"\n\n");
}