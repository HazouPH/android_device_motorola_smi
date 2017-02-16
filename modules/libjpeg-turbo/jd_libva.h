/*
 *  Copyright 2012 Intel Corporation All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef JDAPI_BACKEND_H
#define JDAPI_BACKEND_H

#include "JPEGDecoder_libjpeg_wrapper.h"

typedef struct j_context_list_decoder_t {
  j_decompress_ptr cinfo;
  jd_libva_struct * jd_libva_ptr;
  struct j_context_list_decoder_t * next;
} j_context_list_decoder;

extern j_context_list_decoder *g_context_list_decoder;
extern const char * const jpeg_libva_ext_message_table_decoder[];
extern const char * const * jpeg_message_table_temp_decoder;
extern pthread_mutex_t mutex_decoder;

GLOBAL(void) jpeg_CreateDecompress_hw (j_decompress_ptr cinfo);

GLOBAL(void) jpeg_destroy_decompress_hw (j_decompress_ptr cinfo);
GLOBAL(void) jpeg_destroy_decompress_libva (j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr);
GLOBAL(void) jpeg_destroy_decompress_native (j_decompress_ptr cinfo);

GLOBAL(void) jpeg_abort_decompress_hw (j_decompress_ptr cinfo);
GLOBAL(void) jpeg_abort_decompress_libva (j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr);
GLOBAL(void) jpeg_abort_decompress_native (j_decompress_ptr cinfo);

GLOBAL(int) jpeg_read_header_hw (j_decompress_ptr cinfo, boolean require_image);
GLOBAL(int) jpeg_read_header_libva (j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr);
GLOBAL(int) jpeg_read_header_native (j_decompress_ptr cinfo, boolean require_image);

GLOBAL(boolean) jpeg_finish_decompress_hw (j_decompress_ptr cinfo);
GLOBAL(boolean) jpeg_finish_decompress_libva (j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr);
GLOBAL(boolean) jpeg_finish_decompress_native (j_decompress_ptr cinfo);

GLOBAL(boolean) jpeg_start_decompress_hw (j_decompress_ptr cinfo);
GLOBAL(boolean) jpeg_start_decompress_libva (j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr);
GLOBAL(boolean) jpeg_start_decompress_native (j_decompress_ptr cinfo);

GLOBAL(boolean) jpeg_start_tile_decompress_hw (j_decompress_ptr cinfo);
GLOBAL(boolean) jpeg_start_tile_decompress_libva (j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr);
GLOBAL(boolean) jpeg_start_tile_decompress_native (j_decompress_ptr cinfo);

GLOBAL(JDIMENSION) jpeg_read_scanlines_hw (j_decompress_ptr cinfo, JSAMPARRAY scanlines, JDIMENSION max_lines);
GLOBAL(JDIMENSION) jpeg_read_scanlines_libva (j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr, JSAMPARRAY scanlines, JDIMENSION max_lines);
GLOBAL(JDIMENSION) jpeg_read_scanlines_native (j_decompress_ptr cinfo, JSAMPARRAY scanlines, JDIMENSION max_lines);

GLOBAL(void) jpeg_create_huffman_index_hw(j_decompress_ptr cinfo, huffman_index *index);
GLOBAL(void) jpeg_create_huffman_index_native(j_decompress_ptr cinfo, huffman_index *index);

GLOBAL(boolean) jpeg_build_huffman_index_hw(j_decompress_ptr cinfo, huffman_index *index);

GLOBAL(void) jpeg_destroy_huffman_index_hw(huffman_index *index);
GLOBAL(void) jpeg_destroy_huffman_index_native(huffman_index *index);

GLOBAL(void) jpeg_init_read_tile_scanline_hw (j_decompress_ptr cinfo, huffman_index *index, int *start_x, int *start_y, int *width, int *height);

GLOBAL(JDIMENSION) jpeg_read_tile_scanline_hw (j_decompress_ptr cinfo, huffman_index *index, JSAMPARRAY scanlines);

j_context_list_decoder * append_context_list_decoder (j_context_list_decoder * head, j_context_list_decoder * list);
j_context_list_decoder * remove_context_list_decoder (j_context_list_decoder * head, j_context_list_decoder * list);
j_context_list_decoder * get_context_list_decoder_by_cinfo (j_context_list_decoder * head, j_decompress_ptr cinfo);

#endif //JDAPI_BACKEND_H

