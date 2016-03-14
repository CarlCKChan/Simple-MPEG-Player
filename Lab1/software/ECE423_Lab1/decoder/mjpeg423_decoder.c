//
//  mjpeg423_decoder.c
//  mjpeg423app
//
//  Created by Rodolfo Pellizzoni on 12/24/13.
//  Copyright (c) 2013 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/mjpeg423_types.h"
#include "mjpeg423_decoder.h"
#include "../common/util.h"
#include "../sd_card.h"
#include "../board_diag.h"
#include "altera_up_avalon_video_pixel_buffer_dma.h"


#define HEADER_OFFSET (5*sizeof(uint32_t))
//main decoder function

typedef struct frame_struct {
uint32_t frame_size;
uint32_t frame_type;
uint32_t Ysize;
uint32_t Cbsize;
uint32_t* Ybitstream;
} frame;

void mjpeg423_decode(const char* filename_in)
{
	uint8_t* filebuffer;
    //header and payload info
    uint32_t num_frames, w_size, h_size, num_iframes, payload_size;
    uint32_t Ysize, Cbsize, frame_size, frame_type;
    frame currentFrame;
    int x,y;


    alt_up_pixel_buffer_dma_dev * pixel_buf_dev;
    //file streams
    FILE* file_in;
    //if((file_in = alt_up_sd_card_fopen("v3fps.mpg", 0)) == NULL) error_and_exit("cannot open input file");
    //char* filename_out = malloc(strlen(filenamebase_out)+1);
    //strcpy(filename_out, filenamebase_out);
    
    pixel_buf_dev = alt_up_pixel_buffer_dma_open_dev(VIDEO_PIXEL_BUFFER_DMA_0_NAME);
    if ( pixel_buf_dev == NULL){
    	error_and_exit ("Error: could not open pixel buffer device \n");
    }

    short int file_handle = alt_up_sd_card_fopen(filename_in, 0);
	if(file_handle == -1){
		error_and_exit("Cannot find file\n");
	}

	//create list of sectors
	if(!sd_card_create_sectors_list(file_handle)){
		error_and_exit("Cannot create sectors list\n");
	}

	//create filebuffer to hold entire file
	printf("File size %d\n",sd_card_file_size(file_handle));

	int sectors_num = ceil(sd_card_file_size(file_handle)/512.0);
	if((filebuffer = malloc(sectors_num*512)) == NULL){
		error_and_exit("Cannot allocate filebuffer\n");
	}

	//read all sectors into the filebuffer
	uint8_t* pos = filebuffer;
	for(int i = 0; i < sectors_num; i++){
	  sd_card_start_read_sector(i);
	  if(!sd_card_wait_read_sector()){
		  printf("Cannot read %d-th sector\n", i);
		  return -1;
		}
	  //move sector to filebuffer 32bits at a time
	  for(int j = 0; j < 512; j+=4){
		  *((uint32_t*)(pos+j)) = IORD_32DIRECT(buffer_memory, j);
	  }
	  pos+=512;
	}

	//close file
	if(!alt_up_sd_card_fclose(file_handle)){
	  printf("Failed to close the file\n");
	}

    //read header
    memcpy(&num_frames, filebuffer, sizeof(uint32_t));
    memcpy(&w_size, filebuffer+4, sizeof(uint32_t));
    memcpy(&h_size, filebuffer+8, sizeof(uint32_t));
    memcpy(&num_iframes, filebuffer+12, sizeof(uint32_t));
    memcpy(&payload_size, filebuffer+16, sizeof(uint32_t));
    
    DEBUG_PRINT_ARG("num_frames %u\n", num_iframes)
    DEBUG_PRINT_ARG("payload_size %u\n", payload_size)

    int hCb_size = h_size/8;           //number of chrominance blocks
    int wCb_size = w_size/8;
    int hYb_size = h_size/8;           //number of luminance blocks. Same as chrominance in the sample app
    int wYb_size = w_size/8;
    
    //trailer structure
    iframe_trailer_t* trailer = malloc(sizeof(iframe_trailer_t)*num_frames);
    
    //main data structures. See lab manual for explanation
    rgb_pixel_t* rgbblock;
    if((rgbblock = malloc(w_size*h_size*sizeof(rgb_pixel_t)))==NULL) error_and_exit("cannot allocate rgbblock");
    color_block_t* Yblock;
    if((Yblock = malloc(hYb_size * wYb_size * 64))==NULL) error_and_exit("cannot allocate Yblock");
    color_block_t* Cbblock;
    if((Cbblock = malloc(hCb_size * wCb_size * 64))==NULL) error_and_exit("cannot allocate Cbblock");
    color_block_t* Crblock;
    if((Crblock = malloc(hCb_size * wCb_size * 64))==NULL) error_and_exit("cannot allocate Crblock");;
    dct_block_t* YDCAC;
    if((YDCAC = malloc(hYb_size * wYb_size * 64 * sizeof(DCTELEM)))==NULL) error_and_exit("cannot allocate YDCAC");
    dct_block_t* CbDCAC;
    if((CbDCAC = malloc(hCb_size * wCb_size * 64 * sizeof(DCTELEM)))==NULL) error_and_exit("cannot allocate CbDCAC");
    dct_block_t* CrDCAC;
    if((CrDCAC = malloc(hCb_size * wCb_size * 64 * sizeof(DCTELEM)))==NULL) error_and_exit("cannot allocate CrDCAC");
    //Ybitstream is assigned a size sufficient to hold all bistreams
    //the bitstream is then read from the file into Ybitstream
    //the remaining pointers simply point to the beginning of the Cb and Cr streams within Ybitstream
    uint8_t* Ybitstream;
    if((Ybitstream = malloc(hYb_size * wYb_size * 64 * sizeof(DCTELEM) + 2 * hCb_size * wCb_size * 64 * sizeof(DCTELEM)))==NULL) error_and_exit("cannot allocate bitstream");
    uint8_t* Cbbitstream;
    uint8_t* Crbitstream;
    
    //read trailer. Note: the trailer information is not used in the sample decoder app
    //set file to beginning of trailer
    //if(fseek(file_in, 5 * sizeof(uint32_t) + payload_size, SEEK_SET) != 0) error_and_exit("cannot seek into file");
    for(int count = 0; count < num_iframes; count+=2){
        memcpy(&(trailer[count].frame_index), filebuffer+HEADER_OFFSET+payload_size+(count*sizeof(uint32_t)), sizeof(uint32_t));
        memcpy(&(trailer[count].frame_position), filebuffer+HEADER_OFFSET+payload_size+((count+1)*sizeof(uint32_t)), sizeof(uint32_t));

        //if(fread(&(trailer[count].frame_index), sizeof(uint32_t), 1, file_in) != 1) error_and_exit("cannot read input file");
        //if(fread(&(trailer[count].frame_position), sizeof(uint32_t), 1, file_in) != 1) error_and_exit("cannot read input file");
        DEBUG_PRINT_ARG("I frame index %u, ", trailer[count].frame_index)
        DEBUG_PRINT_ARG("position %u\n", trailer[count].frame_position)
    }
    //set it back to beginning of payload
    //if(fseek(file_in,5 * sizeof(uint32_t),SEEK_SET) != 0) error_and_exit("cannot seek into file");
    
    
    //read and decode frames
    for(int frame_byte_counter = HEADER_OFFSET; frame_byte_counter < payload_size;){
        //DEBUG_PRINT_ARG("\nFrame #%u\n",)
        
        //read frame payload
		memcpy(&(frame_size), filebuffer+frame_byte_counter, sizeof(uint32_t));
        frame_byte_counter+=sizeof(uint32_t);
        DEBUG_PRINT_ARG("Frame_size %u\n",frame_size)
		memcpy(&(frame_type), filebuffer+frame_byte_counter, sizeof(uint32_t));
        frame_byte_counter+=sizeof(uint32_t);
		memcpy(&(Ysize), filebuffer+frame_byte_counter, sizeof(uint32_t));
		frame_byte_counter+=sizeof(uint32_t);
		memcpy(&(Cbsize), filebuffer+frame_byte_counter, sizeof(uint32_t));
		frame_byte_counter+=sizeof(uint32_t);
		memcpy(Ybitstream, filebuffer+frame_byte_counter, frame_size - 4 * sizeof(uint32_t));
		frame_byte_counter += (frame_size - 4 * sizeof(uint32_t));

        /*if(fread(&frame_size, sizeof(uint32_t), 1, file_in) != 1) error_and_exit("cannot read input file");

        if(fread(&frame_type, sizeof(uint32_t), 1, file_in) != 1) error_and_exit("cannot read input file");
        DEBUG_PRINT_ARG("Frame_type %u\n",frame_type)
        if(fread(&Ysize, sizeof(uint32_t), 1, file_in) != 1) error_and_exit("cannot read input file");
        if(fread(&Cbsize, sizeof(uint32_t), 1, file_in) != 1) error_and_exit("cannot read input file");
        if(fread(Ybitstream, 1, frame_size - 4 * sizeof(uint32_t), file_in) != (frame_size - 4 * sizeof(uint32_t))) 
            error_and_exit("cannot read input file");
            */
        //set the Cb and Cr bitstreams to point to the right location
        Cbbitstream = Ybitstream + Ysize;
        Crbitstream = Cbbitstream + Cbsize;
        
        //lossless decoding
        lossless_decode(hYb_size*wYb_size, Ybitstream, YDCAC, Yquant, frame_type);
        lossless_decode(hCb_size*wCb_size, Cbbitstream, CbDCAC, Cquant, frame_type);
        lossless_decode(hCb_size*wCb_size, Crbitstream, CrDCAC, Cquant, frame_type);
        
        //fdct
        for(int b = 0; b < hYb_size*wYb_size; b++) idct(YDCAC[b], Yblock[b]);
        for(int b = 0; b < hCb_size*wCb_size; b++) idct(CbDCAC[b], Cbblock[b]);
        for(int b = 0; b < hCb_size*wCb_size; b++) idct(CrDCAC[b], Crblock[b]);
        
        //ybcbr to rgb conversion
        while(alt_up_pixel_buffer_dma_check_swap_buffers_status(pixel_buf_dev));
        for(int b = 0; b < hCb_size*wCb_size; b++) 
            ycbcr_to_rgb(b/wCb_size*8, b%wCb_size*8, w_size, Yblock[b], Cbblock[b], Crblock[b], (rgb_pixel_t*)(pixel_buf_dev->back_buffer_start_address));
        
        //open and write bmp file
        //long pos = strlen(filename_out) - 8;      //this assumes the namebase is in the format name0000.bmp
        //filename_out[pos] = (char)(frame_index/1000) + '0';
        //filename_out[pos+1] = (char)(frame_index/100%10) + '0';
        //filename_out[pos+2] = (char)(frame_index/10%10) + '0';
        //filename_out[pos+3] = (char)(frame_index%10) + '0';
        //encode_bmp(rgbblock, w_size, h_size, filename_out);
        

//        for (x = 0; x < w_size; x++ ){
//        	for(y = 0; y< h_size; y++){
//        		 alt_up_pixel_buffer_dma_draw(pixel_buf_dev, rgbblock[x+y], x, y);
//        	}
//        }
        //alt_up_pixel_buffer_dma_clear_screen(pixel_buf_dev);
        //alt_up_pixel_buffer_draw_box(pixel_buf_dev, 0, 0, 319, 239, 0x001F, 0);
		alt_up_pixel_buffer_dma_swap_buffers(pixel_buf_dev);


    } //end frame iteration
    
    DEBUG_PRINT("\nDecoder done.\n\n\n")
    
    //close down
    //fclose(file_in);
    alt_up_sd_card_fclose(file_handle);
    free(rgbblock); 
    free(Yblock);
    free(Cbblock);
    free(Crblock);
    free(YDCAC);
    free(CbDCAC);
    free(CrDCAC);
    free(Ybitstream);

}
