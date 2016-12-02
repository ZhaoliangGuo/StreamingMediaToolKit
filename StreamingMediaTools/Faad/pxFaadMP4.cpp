/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2005 M. Bakker, Nero AG, http://www.nero.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** The "appropriate copyright message" mentioned in section 2c of the GPLv2
** must read: "Code from FAAD2 is copyright (c) Nero AG, www.nero.com"
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Nero AG through Mpeg4AAClicense@nero.com.
**
** $Id: mp4.c,v 1.40 2009/02/06 03:39:58 menno Exp $
**/

// comment by gzl @20160525
// faad是为了解析aacConfig
// 相关函数为NeAACDecAudioSpecificConfig

#include "pxFAADCommonDef.h"

#include <stdlib.h>

#include "pxFaadBits.h"
#include "pxFaadMP4.h"
#include <memory.h>
#include <string.h>

//static unsigned char program_config_element(program_config *pce, bitfile *ld);

/* Table 1.6.1 */
char pxAACDecodeAudioSpecificConfig(unsigned char *pBuffer,
                                 unsigned long buffer_size,
                                 mp4AudioSpecificConfig *mp4ASC)
{
    return AudioSpecificConfig2(pBuffer, buffer_size, mp4ASC, NULL, 0);
}

signed char AudioSpecificConfigFromBitfile(bitfile *ld,
                            mp4AudioSpecificConfig *mp4ASC,
                            program_config *pce, 
							unsigned long buffer_size, 
							unsigned char short_form)
{
    signed char result = 0;
    unsigned long startpos = faad_get_processed_bits(ld);
#ifdef SBR_DEC
    signed char bits_to_decode = 0;
#endif

    if (mp4ASC == NULL)
        return -8;

    memset(mp4ASC, 0, sizeof(mp4AudioSpecificConfig));

    mp4ASC->objectTypeIndex = (unsigned char)faad_getbits(ld, 5
        DEBUGVAR(1,1,"parse_audio_decoder_specific_info(): ObjectTypeIndex"));

    mp4ASC->samplingFrequencyIndex = (unsigned char)faad_getbits(ld, 4
        DEBUGVAR(1,2,"parse_audio_decoder_specific_info(): SamplingFrequencyIndex"));
	if(mp4ASC->samplingFrequencyIndex==0x0f)
		faad_getbits(ld, 24);

    mp4ASC->channelsConfiguration = (unsigned char)faad_getbits(ld, 4
        DEBUGVAR(1,3,"parse_audio_decoder_specific_info(): ChannelsConfiguration"));

    mp4ASC->samplingFrequency = get_sample_rate(mp4ASC->samplingFrequencyIndex);

    if (ObjectTypesTable[mp4ASC->objectTypeIndex] != 1)
    {
        return -1;
    }

    if (mp4ASC->samplingFrequency == 0)
    {
        return -2;
    }

    if (mp4ASC->channelsConfiguration > 7)
    {
        return -3;
    }

#if (defined(PS_DEC) || defined(DRM_PS))
    /* check if we have a mono file */
    //if (mp4ASC->channelsConfiguration == 1)
    //{
    //    /* upMatrix to 2 channels for implicit signalling of PS */
    //    mp4ASC->channelsConfiguration = 2;
    //}
#endif

#ifdef SBR_DEC
    mp4ASC->sbr_present_flag = -1;
    if (mp4ASC->objectTypeIndex == 5)
    {
        unsigned char tmp;

        mp4ASC->sbr_present_flag = 1;
        tmp = (unsigned char)faad_getbits(ld, 4
            DEBUGVAR(1,5,"parse_audio_decoder_specific_info(): extensionSamplingFrequencyIndex"));
        /* check for downsampled SBR */
        if (tmp == mp4ASC->samplingFrequencyIndex)
            mp4ASC->downSampledSBR = 1;
        mp4ASC->samplingFrequencyIndex = tmp;
        if (mp4ASC->samplingFrequencyIndex == 15)
        {
            mp4ASC->samplingFrequency = (unsigned long)faad_getbits(ld, 24
                DEBUGVAR(1,6,"parse_audio_decoder_specific_info(): extensionSamplingFrequencyIndex"));
        } else {
            mp4ASC->samplingFrequency = get_sample_rate(mp4ASC->samplingFrequencyIndex);
        }
        mp4ASC->objectTypeIndex = (unsigned char)faad_getbits(ld, 5
            DEBUGVAR(1,7,"parse_audio_decoder_specific_info(): ObjectTypeIndex"));
    }
#endif

    /* get GASpecificConfig */
//    if (mp4ASC->objectTypeIndex == 1 || mp4ASC->objectTypeIndex == 2 ||
//        mp4ASC->objectTypeIndex == 3 || mp4ASC->objectTypeIndex == 4 ||
//        mp4ASC->objectTypeIndex == 6 || mp4ASC->objectTypeIndex == 7)
//    {
//        result = GASpecificConfig(ld, mp4ASC, pce);
//
//#ifdef ERROR_RESILIENCE
//    } 
//	else if (mp4ASC->objectTypeIndex >= ER_OBJECT_START) 
//	{ /* ER */
//        result = GASpecificConfig(ld, mp4ASC, pce);
//        mp4ASC->epConfig = (unsigned char)faad_getbits(ld, 2
//            DEBUGVAR(1,143,"parse_audio_decoder_specific_info(): epConfig"));
//
//        if (mp4ASC->epConfig != 0)
//            result = -5;
//#endif
//
//    } 
//	else 
//	{
//        result = -4;
//    }

#ifdef SSR_DEC
    /* shorter frames not allowed for SSR */
    if ((mp4ASC->objectTypeIndex == 4) && mp4ASC->frameLengthFlag)
        return -6;
#endif


#ifdef SBR_DEC
    if(short_form)
        bits_to_decode = 0;
    else
		bits_to_decode = (signed char)(buffer_size*8 - (startpos-faad_get_processed_bits(ld)));

    if ((mp4ASC->objectTypeIndex != 5) && (bits_to_decode >= 16))
    {
        int16_t syncExtensionType = (int16_t)faad_getbits(ld, 11
            DEBUGVAR(1,9,"parse_audio_decoder_specific_info(): syncExtensionType"));

        if (syncExtensionType == 0x2b7)
        {
            unsigned char tmp_OTi = (unsigned char)faad_getbits(ld, 5
                DEBUGVAR(1,10,"parse_audio_decoder_specific_info(): extensionAudioObjectType"));

            if (tmp_OTi == 5)
            {
                mp4ASC->sbr_present_flag = (unsigned char)faad_get1bit(ld
                    DEBUGVAR(1,11,"parse_audio_decoder_specific_info(): sbr_present_flag"));

                if (mp4ASC->sbr_present_flag)
                {
                    unsigned char tmp;

					/* Don't set OT to SBR until checked that it is actually there */
					mp4ASC->objectTypeIndex = tmp_OTi;

                    tmp = (unsigned char)faad_getbits(ld, 4
                        DEBUGVAR(1,12,"parse_audio_decoder_specific_info(): extensionSamplingFrequencyIndex"));

                    /* check for downsampled SBR */
                    if (tmp == mp4ASC->samplingFrequencyIndex)
                        mp4ASC->downSampledSBR = 1;
                    mp4ASC->samplingFrequencyIndex = tmp;

                    if (mp4ASC->samplingFrequencyIndex == 15)
                    {
                        mp4ASC->samplingFrequency = (unsigned long)faad_getbits(ld, 24
                            DEBUGVAR(1,13,"parse_audio_decoder_specific_info(): extensionSamplingFrequencyIndex"));
                    } else {
                        mp4ASC->samplingFrequency = get_sample_rate(mp4ASC->samplingFrequencyIndex);
                    }
                }
            }
        }
    }

    /* no SBR signalled, this could mean either implicit signalling or no SBR in this file */
    /* MPEG specification states: assume SBR on files with samplerate <= 24000 Hz */

	//if (mp4ASC->sbr_present_flag == -1)
 //   {
 //       if (mp4ASC->samplingFrequency <= 24000)
 //       {
 //           mp4ASC->samplingFrequency *= 2;
 //           mp4ASC->forceUpSampling = 1;
 //       } 
	//	else /* > 24000*/ 
	//	{
 //           mp4ASC->downSampledSBR = 1;
 //       }
 //   }


#endif

    faad_endbits(ld);

    return result;
}

signed char AudioSpecificConfig2(unsigned char *pBuffer,
                            unsigned long buffer_size,
                            mp4AudioSpecificConfig *mp4ASC,
                            program_config *pce,
                            unsigned char short_form)
{
    unsigned char ret = 0;
    bitfile ld;
    faad_initbits(&ld, pBuffer, buffer_size);
    faad_byte_align(&ld);
    ret = AudioSpecificConfigFromBitfile(&ld, mp4ASC, pce, buffer_size, short_form);
    faad_endbits(&ld);
    return ret;
}

///* Table 4.4.1 */
//signed char GASpecificConfig(bitfile *ld, mp4AudioSpecificConfig *mp4ASC,
//                        SPxProgramConfig *pce_out)
//{
//    SPxProgramConfig pce;
//
//    /* 1024 or 960 */
//    mp4ASC->frameLengthFlag = faad_get1bit(ld
//        DEBUGVAR(1,138,"GASpecificConfig(): FrameLengthFlag"));
//#ifndef ALLOW_SMALL_FRAMELENGTH
//    if (mp4ASC->frameLengthFlag == 1)
//        return -3;
//#endif
//
//    mp4ASC->dependsOnCoreCoder = faad_get1bit(ld
//        DEBUGVAR(1,139,"GASpecificConfig(): DependsOnCoreCoder"));
//    if (mp4ASC->dependsOnCoreCoder == 1)
//    {
//        mp4ASC->coreCoderDelay = (uint16_t)faad_getbits(ld, 14
//            DEBUGVAR(1,140,"GASpecificConfig(): CoreCoderDelay"));
//    }
//
//    mp4ASC->extensionFlag = faad_get1bit(ld DEBUGVAR(1,141,"GASpecificConfig(): ExtensionFlag"));
//    if (mp4ASC->channelsConfiguration == 0)
//    {
//        if (program_config_element(&pce, ld))
//            return -3;
//        //mp4ASC->channelsConfiguration = pce.channels;
//
//        if (pce_out != NULL)
//            memcpy(pce_out, &pce, sizeof(SPxProgramConfig));
//
//        /*
//        if (pce.num_valid_cc_elements)
//            return -3;
//        */
//    }
//
//#ifdef ERROR_RESILIENCE
//    if (mp4ASC->extensionFlag == 1)
//    {
//        /* Error resilience not supported yet */
//        if (mp4ASC->objectTypeIndex >= ER_OBJECT_START)
//        {
//            mp4ASC->aacSectionDataResilienceFlag = faad_get1bit(ld
//                DEBUGVAR(1,144,"GASpecificConfig(): aacSectionDataResilienceFlag"));
//            mp4ASC->aacScalefactorDataResilienceFlag = faad_get1bit(ld
//                DEBUGVAR(1,145,"GASpecificConfig(): aacScalefactorDataResilienceFlag"));
//            mp4ASC->aacSpectralDataResilienceFlag = faad_get1bit(ld
//                DEBUGVAR(1,146,"GASpecificConfig(): aacSpectralDataResilienceFlag"));
//        }
//        /* 1 bit: extensionFlag3 */
//        faad_getbits(ld, 1);
//	}
//#endif
//
//    return 0;
//}
//
//#define MAX_CHANNELS        64
//
///* Table 4.4.2 */
///* An MPEG-4 Audio decoder is only required to follow the Program
//   Configuration Element in GASpecificConfig(). The decoder shall ignore
//   any Program Configuration Elements that may occur in raw data blocks.
//   PCEs transmitted in raw data blocks cannot be used to convey decoder
//   configuration information.
//*/
//static unsigned char program_config_element(SPxProgramConfig *pce, bitfile *ld)
//{
//    unsigned char i;
//
//    memset(pce, 0, sizeof(SPxProgramConfig));
//
//    pce->channels = 0;
//
//    pce->element_instance_tag = (unsigned char)faad_getbits(ld, 4
//        DEBUGVAR(1,10,"program_config_element(): element_instance_tag"));
//
//    pce->object_type = (unsigned char)faad_getbits(ld, 2
//        DEBUGVAR(1,11,"program_config_element(): object_type"));
//    pce->sf_index = (unsigned char)faad_getbits(ld, 4
//        DEBUGVAR(1,12,"program_config_element(): sf_index"));
//    pce->num_front_channel_elements = (unsigned char)faad_getbits(ld, 4
//        DEBUGVAR(1,13,"program_config_element(): num_front_channel_elements"));
//    pce->num_side_channel_elements = (unsigned char)faad_getbits(ld, 4
//        DEBUGVAR(1,14,"program_config_element(): num_side_channel_elements"));
//    pce->num_back_channel_elements = (unsigned char)faad_getbits(ld, 4
//        DEBUGVAR(1,15,"program_config_element(): num_back_channel_elements"));
//    pce->num_lfe_channel_elements = (unsigned char)faad_getbits(ld, 2
//        DEBUGVAR(1,16,"program_config_element(): num_lfe_channel_elements"));
//    pce->num_assoc_data_elements = (unsigned char)faad_getbits(ld, 3
//        DEBUGVAR(1,17,"program_config_element(): num_assoc_data_elements"));
//    pce->num_valid_cc_elements = (unsigned char)faad_getbits(ld, 4
//        DEBUGVAR(1,18,"program_config_element(): num_valid_cc_elements"));
//
//    pce->mono_mixdown_present = faad_get1bit(ld
//        DEBUGVAR(1,19,"program_config_element(): mono_mixdown_present"));
//    if (pce->mono_mixdown_present == 1)
//    {
//        pce->mono_mixdown_element_number = (unsigned char)faad_getbits(ld, 4
//            DEBUGVAR(1,20,"program_config_element(): mono_mixdown_element_number"));
//    }
//
//    pce->stereo_mixdown_present = faad_get1bit(ld
//        DEBUGVAR(1,21,"program_config_element(): stereo_mixdown_present"));
//    if (pce->stereo_mixdown_present == 1)
//    {
//        pce->stereo_mixdown_element_number = (unsigned char)faad_getbits(ld, 4
//            DEBUGVAR(1,22,"program_config_element(): stereo_mixdown_element_number"));
//    }
//
//    pce->matrix_mixdown_idx_present = faad_get1bit(ld
//        DEBUGVAR(1,23,"program_config_element(): matrix_mixdown_idx_present"));
//    if (pce->matrix_mixdown_idx_present == 1)
//    {
//        pce->matrix_mixdown_idx = (unsigned char)faad_getbits(ld, 2
//            DEBUGVAR(1,24,"program_config_element(): matrix_mixdown_idx"));
//        pce->pseudo_surround_enable = faad_get1bit(ld
//            DEBUGVAR(1,25,"program_config_element(): pseudo_surround_enable"));
//    }
//
//    for (i = 0; i < pce->num_front_channel_elements; i++)
//    {
//        pce->front_element_is_cpe[i] = faad_get1bit(ld
//            DEBUGVAR(1,26,"program_config_element(): front_element_is_cpe"));
//        pce->front_element_tag_select[i] = (unsigned char)faad_getbits(ld, 4
//            DEBUGVAR(1,27,"program_config_element(): front_element_tag_select"));
//
//        if (pce->front_element_is_cpe[i] & 1)
//        {
//            pce->cpe_channel[pce->front_element_tag_select[i]] = pce->channels;
//            pce->num_front_channels += 2;
//            pce->channels += 2;
//        } else {
//            pce->sce_channel[pce->front_element_tag_select[i]] = pce->channels;
//            pce->num_front_channels++;
//            pce->channels++;
//        }
//    }
//
//    for (i = 0; i < pce->num_side_channel_elements; i++)
//    {
//        pce->side_element_is_cpe[i] = faad_get1bit(ld
//            DEBUGVAR(1,28,"program_config_element(): side_element_is_cpe"));
//        pce->side_element_tag_select[i] = (unsigned char)faad_getbits(ld, 4
//            DEBUGVAR(1,29,"program_config_element(): side_element_tag_select"));
//
//        if (pce->side_element_is_cpe[i] & 1)
//        {
//            pce->cpe_channel[pce->side_element_tag_select[i]] = pce->channels;
//            pce->num_side_channels += 2;
//            pce->channels += 2;
//        } else {
//            pce->sce_channel[pce->side_element_tag_select[i]] = pce->channels;
//            pce->num_side_channels++;
//            pce->channels++;
//        }
//    }
//
//    for (i = 0; i < pce->num_back_channel_elements; i++)
//    {
//        pce->back_element_is_cpe[i] = faad_get1bit(ld
//            DEBUGVAR(1,30,"program_config_element(): back_element_is_cpe"));
//        pce->back_element_tag_select[i] = (unsigned char)faad_getbits(ld, 4
//            DEBUGVAR(1,31,"program_config_element(): back_element_tag_select"));
//
//        if (pce->back_element_is_cpe[i] & 1)
//        {
//            pce->cpe_channel[pce->back_element_tag_select[i]] = pce->channels;
//            pce->channels += 2;
//            pce->num_back_channels += 2;
//        } else {
//            pce->sce_channel[pce->back_element_tag_select[i]] = pce->channels;
//            pce->num_back_channels++;
//            pce->channels++;
//        }
//    }
//
//    for (i = 0; i < pce->num_lfe_channel_elements; i++)
//    {
//        pce->lfe_element_tag_select[i] = (unsigned char)faad_getbits(ld, 4
//            DEBUGVAR(1,32,"program_config_element(): lfe_element_tag_select"));
//
//        pce->sce_channel[pce->lfe_element_tag_select[i]] = pce->channels;
//        pce->num_lfe_channels++;
//        pce->channels++;
//    }
//
//    for (i = 0; i < pce->num_assoc_data_elements; i++)
//        pce->assoc_data_element_tag_select[i] = (unsigned char)faad_getbits(ld, 4
//        DEBUGVAR(1,33,"program_config_element(): assoc_data_element_tag_select"));
//
//    for (i = 0; i < pce->num_valid_cc_elements; i++)
//    {
//        pce->cc_element_is_ind_sw[i] = faad_get1bit(ld
//            DEBUGVAR(1,34,"program_config_element(): cc_element_is_ind_sw"));
//        pce->valid_cc_element_tag_select[i] = (unsigned char)faad_getbits(ld, 4
//            DEBUGVAR(1,35,"program_config_element(): valid_cc_element_tag_select"));
//    }
//
//    faad_byte_align(ld);
//
//    pce->comment_field_bytes = (unsigned char)faad_getbits(ld, 8
//        DEBUGVAR(1,36,"program_config_element(): comment_field_bytes"));
//
//    for (i = 0; i < pce->comment_field_bytes; i++)
//    {
//        pce->comment_field_data[i] = (unsigned char)faad_getbits(ld, 8
//            DEBUGVAR(1,37,"program_config_element(): comment_field_data"));
//    }
//    pce->comment_field_data[i] = 0;
//
//    if (pce->channels > MAX_CHANNELS)
//        return 22;
//
//    return 0;
//}
