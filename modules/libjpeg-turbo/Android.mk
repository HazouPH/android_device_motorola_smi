LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PREBUILT_LIBS := simd/libsimd.a
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)


include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
        $(TARGET_OUT_HEADERS)/ipp

LOCAL_C_INCLUDES += \
    system/core/include

LOCAL_CFLAGS += -DIPP_HUFFMAN

LOCAL_SRC_FILES := \
    jcapimin.c jcapistd.c jccoefct.c jccolor.c jcdctmgr.c jchuff.c \
    jcinit.c jcmainct.c jcmarker.c jcmaster.c jcomapi.c jcparam.c \
    jcphuff.c jcprepct.c jcsample.c jctrans.c jdapimin.c jdapistd.c \
    jdatadst.c jdatasrc.c jdcoefct.c jdcolor.c jddctmgr.c jdhuff.c \
    jdinput.c jdmainct.c jdmarker.c jdmaster.c jdmerge.c jdphuff.c \
    jdpostct.c jdsample.c jdtrans.c jerror.c jfdctflt.c jfdctfst.c \
    jfdctint.c jidctflt.c jidctfst.c jidctint.c jidctred.c jquant1.c \
    jquant2.c jutils.c jmemmgr.c simd/jsimd_i386.c

# unbundled branch, built against NDK.
LOCAL_SDK_VERSION := 17
# the original android memory manager.
# use sdcard as libjpeg decoder's backing store
LOCAL_SRC_FILES += \
    jmem-android.c

LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays
# enable tile based decode
LOCAL_CFLAGS += -DANDROID_TILE_BASED_DECODE

LOCAL_STATIC_LIBRARIES += libsimd
LOCAL_STATIC_LIBRARIES += \
        libippj \
        libippi \
        libipps \
        libippcore

LOCAL_SHARED_LIBRARIES += liblog libutils
LOCAL_MODULE := libjpeg-turbo
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES += \
        $(TARGET_OUT_HEADERS)/ipp

LOCAL_CFLAGS += -DIPP_HUFFMAN

LOCAL_SRC_FILES := \
    jcapimin.c jcapistd.c jccoefct.c jccolor.c jcdctmgr.c jchuff.c \
    jcinit.c jcmainct.c jcmarker.c jcmaster.c jcomapi.c jcparam.c \
    jcphuff.c jcprepct.c jcsample.c jctrans.c jdapimin.c jdapistd.c \
    jdatadst.c jdatasrc.c jdcoefct.c jdcolor.c jddctmgr.c jdhuff.c \
    jdinput.c jdmainct.c jdmarker.c jdmaster.c jdmerge.c jdphuff.c \
    jdpostct.c jdsample.c jdtrans.c jerror.c jfdctflt.c jfdctfst.c \
    jfdctint.c jidctflt.c jidctfst.c jidctint.c jidctred.c jquant1.c \
    jquant2.c jutils.c jmemmgr.c simd/jsimd_i386.c

# unbundled branch, built against NDK.
LOCAL_SDK_VERSION := 17
# the original android memory manager.
# use sdcard as libjpeg decoder's backing store
LOCAL_SRC_FILES += \
    jmem-android.c

LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays
# enable tile based decode
LOCAL_CFLAGS += -DANDROID_TILE_BASED_DECODE

LOCAL_STATIC_LIBRARIES += libsimd
LOCAL_STATIC_LIBRARIES += \
        libippj \
        libippi \
        libipps \
        libippcore
LOCAL_SHARED_LIBRARIES += liblog libutils
LOCAL_C_INCLUDES += \
    system/core/include
LOCAL_CFLAGS += -Wno-multichar -DLOG_TAG=\"libjpeg-turbo-static\"
LOCAL_CFLAGS += \
    -Djcopy_block_row=jt_jcopy_block_row	\
    -Djcopy_sample_rows=jt_jcopy_sample_rows	\
    -Djdiv_round_up=jt_jdiv_round_up	\
    -Djget_input_stream_position=jt_jget_input_stream_position	\
    -Djinit_1pass_quantizer=jt_jinit_1pass_quantizer	\
    -Djinit_2pass_quantizer=jt_jinit_2pass_quantizer	\
    -Djinit_arith_decoder=jt_jinit_arith_decoder	\
    -Djinit_arith_encoder=jt_jinit_arith_encoder	\
    -Djinit_c_coef_controller=jt_jinit_c_coef_controller	\
    -Djinit_c_main_controller=jt_jinit_c_main_controller	\
    -Djinit_c_master_control=jt_jinit_c_master_control	\
    -Djinit_c_prep_controller=jt_jinit_c_prep_controller	\
    -Djinit_color_converter=jt_jinit_color_converter	\
    -Djinit_color_deconverter=jt_jinit_color_deconverter	\
    -Djinit_compress_master=jt_jinit_compress_master	\
    -Djinit_d_coef_controller=jt_jinit_d_coef_controller	\
    -Djinit_d_main_controller=jt_jinit_d_main_controller	\
    -Djinit_d_post_controller=jt_jinit_d_post_controller	\
    -Djinit_downsampler=jt_jinit_downsampler	\
    -Djinit_forward_dct=jt_jinit_forward_dct	\
    -Djinit_huff_decoder=jt_jinit_huff_decoder	\
    -Djinit_huff_encoder=jt_jinit_huff_encoder	\
    -Djinit_input_controller=jt_jinit_input_controller	\
    -Djinit_inverse_dct=jt_jinit_inverse_dct	\
    -Djinit_marker_reader=jt_jinit_marker_reader	\
    -Djinit_marker_writer=jt_jinit_marker_writer	\
    -Djinit_master_decompress=jt_jinit_master_decompress	\
    -Djinit_memory_mgr=jt_jinit_memory_mgr	\
    -Djinit_merged_upsampler=jt_jinit_merged_upsampler	\
    -Djinit_phuff_decoder=jt_jinit_phuff_decoder	\
    -Djinit_phuff_encoder=jt_jinit_phuff_encoder	\
    -Djinit_upsampler=jt_jinit_upsampler	\
    -Djmin=jt_jmin	\
    -Djpeg_abort=jt_jpeg_abort	\
    -Djpeg_abort_compress=jt_jpeg_abort_compress	\
    -Djpeg_abort_decompress=jt_jpeg_abort_decompress	\
    -Djpeg_add_quant_table=jt_jpeg_add_quant_table	\
    -Djpeg_alloc_huff_table=jt_jpeg_alloc_huff_table	\
    -Djpeg_alloc_quant_table=jt_jpeg_alloc_quant_table	\
    -Djpeg_aritab=jt_jpeg_aritab	\
    -Djpeg_build_huffman_index=jt_jpeg_build_huffman_index	\
    -Djpeg_calc_jpeg_dimensions=jt_jpeg_calc_jpeg_dimensions	\
    -Djpeg_calc_output_dimensions=jt_jpeg_calc_output_dimensions	\
    -Djpeg_configure_huffman_decoder=jt_jpeg_configure_huffman_decoder	\
    -Djpeg_configure_huffman_decoder_progressive=jt_jpeg_configure_huffman_decoder_progressive	\
    -Djpeg_configure_huffman_index_scan=jt_jpeg_configure_huffman_index_scan	\
    -Djpeg_consume_input=jt_jpeg_consume_input	\
    -Djpeg_copy_critical_parameters=jt_jpeg_copy_critical_parameters	\
    -Djpeg_core_output_dimensions=jt_jpeg_core_output_dimensions	\
    -Djpeg_create_compress=jt_jpeg_create_compress	\
    -Djpeg_create_decompress=jt_jpeg_create_decompress	\
    -Djpeg_create_huffman_index=jt_jpeg_create_huffman_index	\
    -Djpeg_CreateCompress=jt_jpeg_CreateCompress	\
    -Djpeg_CreateDecompress=jt_jpeg_CreateDecompress	\
    -Djpeg_decompress_per_scan_setup=jt_jpeg_decompress_per_scan_setup	\
    -Djpeg_default_colorspace=jt_jpeg_default_colorspace	\
    -Djpeg_default_qtables=jt_jpeg_default_qtables	\
    -Djpeg_destroy=jt_jpeg_destroy	\
    -Djpeg_destroy_compress=jt_jpeg_destroy_compress	\
    -Djpeg_destroy_decompress=jt_jpeg_destroy_decompress	\
    -Djpeg_destroy_huffman_index=jt_jpeg_destroy_huffman_index	\
    -Djpeg_fdct_float=jt_jpeg_fdct_float	\
    -Djpeg_fdct_ifast=jt_jpeg_fdct_ifast	\
    -Djpeg_fdct_islow=jt_jpeg_fdct_islow	\
    -Djpeg_fill_bit_buffer=jt_jpeg_fill_bit_buffer	\
    -Djpeg_finish_compress=jt_jpeg_finish_compress	\
    -Djpeg_finish_decompress=jt_jpeg_finish_decompress	\
    -Djpeg_finish_output=jt_jpeg_finish_output	\
    -Djpeg_free_large=jt_jpeg_free_large	\
    -Djpeg_free_small=jt_jpeg_free_small	\
    -Djpeg_gen_optimal_table=jt_jpeg_gen_optimal_table	\
    -Djpeg_get_huffman_decoder_configuration=jt_jpeg_get_huffman_decoder_configuration	\
    -Djpeg_get_huffman_decoder_configuration_progressive=jt_jpeg_get_huffman_decoder_configuration_progressive	\
    -Djpeg_get_large=jt_jpeg_get_large	\
    -Djpeg_get_small=jt_jpeg_get_small	\
    -Djpeg_has_multiple_scans=jt_jpeg_has_multiple_scans	\
    -Djpeg_huff_decode=jt_jpeg_huff_decode	\
    -Djpeg_idct_10x10=jt_jpeg_idct_10x10	\
    -Djpeg_idct_11x11=jt_jpeg_idct_11x11	\
    -Djpeg_idct_12x12=jt_jpeg_idct_12x12	\
    -Djpeg_idct_13x13=jt_jpeg_idct_13x13	\
    -Djpeg_idct_14x14=jt_jpeg_idct_14x14	\
    -Djpeg_idct_15x15=jt_jpeg_idct_15x15	\
    -Djpeg_idct_16x16=jt_jpeg_idct_16x16	\
    -Djpeg_idct_1x1=jt_jpeg_idct_1x1	\
    -Djpeg_idct_2x2=jt_jpeg_idct_2x2	\
    -Djpeg_idct_3x3=jt_jpeg_idct_3x3	\
    -Djpeg_idct_4x4=jt_jpeg_idct_4x4	\
    -Djpeg_idct_5x5=jt_jpeg_idct_5x5	\
    -Djpeg_idct_6x6=jt_jpeg_idct_6x6	\
    -Djpeg_idct_7x7=jt_jpeg_idct_7x7	\
    -Djpeg_idct_9x9=jt_jpeg_idct_9x9	\
    -Djpeg_idct_float=jt_jpeg_idct_float	\
    -Djpeg_idct_ifast=jt_jpeg_idct_ifast	\
    -Djpeg_idct_islow=jt_jpeg_idct_islow	\
    -Djpeg_init_read_tile_scanline=jt_jpeg_init_read_tile_scanline	\
    -Djpeg_input_complete=jt_jpeg_input_complete	\
    -Djpeg_make_c_derived_tbl=jt_jpeg_make_c_derived_tbl	\
    -Djpeg_make_c_derived_tbl_ipp=jt_jpeg_make_c_derived_tbl_ipp	\
    -Djpeg_make_d_derived_tbl=jt_jpeg_make_d_derived_tbl	\
    -Djpeg_mem_available=jt_jpeg_mem_available	\
    -Djpeg_mem_dest=jt_jpeg_mem_dest	\
    -Djpeg_mem_dest_tj=jt_jpeg_mem_dest_tj	\
    -Djpeg_mem_init=jt_jpeg_mem_init	\
    -Djpeg_mem_src=jt_jpeg_mem_src	\
    -Djpeg_mem_src_tj=jt_jpeg_mem_src_tj	\
    -Djpeg_mem_term=jt_jpeg_mem_term	\
    -Djpeg_new_colormap=jt_jpeg_new_colormap	\
    -Djpeg_open_backing_store=jt_jpeg_open_backing_store	\
    -Djpeg_quality_scaling=jt_jpeg_quality_scaling	\
    -Djpeg_read_coefficients=jt_jpeg_read_coefficients	\
    -Djpeg_read_header=jt_jpeg_read_header	\
    -Djpeg_read_raw_data=jt_jpeg_read_raw_data	\
    -Djpeg_read_scanlines=jt_jpeg_read_scanlines	\
    -Djpeg_read_scanlines_from=jt_jpeg_read_scanlines_from	\
    -Djpeg_read_tile_scanline=jt_jpeg_read_tile_scanline	\
    -Djpeg_resync_to_restart=jt_jpeg_resync_to_restart	\
    -Djpeg_save_markers=jt_jpeg_save_markers	\
    -Djpeg_set_colorspace=jt_jpeg_set_colorspace	\
    -Djpeg_set_defaults=jt_jpeg_set_defaults	\
    -Djpeg_set_linear_quality=jt_jpeg_set_linear_quality	\
    -Djpeg_set_marker_processor=jt_jpeg_set_marker_processor	\
    -Djpeg_set_quality=jt_jpeg_set_quality	\
    -Djpeg_simple_progression=jt_jpeg_simple_progression	\
    -Djpeg_start_compress=jt_jpeg_start_compress	\
    -Djpeg_start_decompress=jt_jpeg_start_decompress	\
    -Djpeg_start_output=jt_jpeg_start_output	\
    -Djpeg_start_tile_decompress=jt_jpeg_start_tile_decompress	\
    -Djpeg_std_error=jt_jpeg_std_error	\
    -Djpeg_stdio_dest=jt_jpeg_stdio_dest	\
    -Djpeg_stdio_src=jt_jpeg_stdio_src	\
    -Djpeg_suppress_tables=jt_jpeg_suppress_tables	\
    -Djpeg_write_coefficients=jt_jpeg_write_coefficients	\
    -Djpeg_write_m_byte=jt_jpeg_write_m_byte	\
    -Djpeg_write_m_header=jt_jpeg_write_m_header	\
    -Djpeg_write_marker=jt_jpeg_write_marker	\
    -Djpeg_write_raw_data=jt_jpeg_write_raw_data	\
    -Djpeg_write_scanlines=jt_jpeg_write_scanlines	\
    -Djpeg_write_tables=jt_jpeg_write_tables	\
    -Djround_up=jt_jround_up	\
    -Djset_input_stream_position=jt_jset_input_stream_position	\
    -Djset_input_stream_position_bit=jt_jset_input_stream_position_bit	\
    -Djzero_far=jt_jzero_far


LOCAL_MODULE := libjpeg-turbo-static
LOCAL_MODULE_TAGS := optional
include $(BUILD_STATIC_LIBRARY)
