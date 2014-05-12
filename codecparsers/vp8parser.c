/* vp8parser.c
 *
 *  Copyright (C) 2013-2014 Intel Corporation
 *    Author: Zhao, Halley<halley.zhao@intel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
/**
 * SECTION:vp8parser
 * @short_description: Convenience library for parsing vp8 video bitstream.
 *
 * For more details about the structures, you can refer to the
 * specifications: VP8-rfc6386.pdf
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "bytereader.h"
#include "vp8parser.h"
#include "dboolhuff.h"
#include "string.h"
#include "log.h"

/* section 13.4 of spec */
static const uint8_t vp8_token_update_probs[4][8][3][11] = {
  {
        {
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {176, 246, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {223, 241, 252, 255, 255, 255, 255, 255, 255, 255, 255},
              {249, 253, 253, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 244, 252, 255, 255, 255, 255, 255, 255, 255, 255},
              {234, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255},
              {253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 246, 254, 255, 255, 255, 255, 255, 255, 255, 255},
              {239, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255},
              {254, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 248, 254, 255, 255, 255, 255, 255, 255, 255, 255},
              {251, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255},
              {251, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255},
              {254, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 254, 253, 255, 254, 255, 255, 255, 255, 255, 255},
              {250, 255, 254, 255, 254, 255, 255, 255, 255, 255, 255},
              {254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
            }
      },
  {
        {
              {217, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {225, 252, 241, 253, 255, 255, 254, 255, 255, 255, 255},
              {234, 250, 241, 250, 253, 255, 253, 254, 255, 255, 255}
            },
        {
              {255, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {223, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255},
              {238, 253, 254, 254, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 248, 254, 255, 255, 255, 255, 255, 255, 255, 255},
              {249, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {247, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255},
              {252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255},
              {253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 254, 253, 255, 255, 255, 255, 255, 255, 255, 255},
              {250, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
            }
      },
  {
        {
              {186, 251, 250, 255, 255, 255, 255, 255, 255, 255, 255},
              {234, 251, 244, 254, 255, 255, 255, 255, 255, 255, 255},
              {251, 251, 243, 253, 254, 255, 254, 255, 255, 255, 255}
            },
        {
              {255, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255},
              {236, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255},
              {251, 253, 253, 254, 254, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255},
              {254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {254, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
            }
      },
  {
        {
              {248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {250, 254, 252, 254, 255, 255, 255, 255, 255, 255, 255},
              {248, 254, 249, 253, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 253, 253, 255, 255, 255, 255, 255, 255, 255, 255},
              {246, 253, 253, 255, 255, 255, 255, 255, 255, 255, 255},
              {252, 254, 251, 254, 254, 255, 255, 255, 255, 255, 255}
            },

        {
              {255, 254, 252, 255, 255, 255, 255, 255, 255, 255, 255},
              {248, 254, 253, 255, 255, 255, 255, 255, 255, 255, 255},
              {253, 255, 254, 254, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 251, 254, 255, 255, 255, 255, 255, 255, 255, 255},
              {245, 251, 254, 255, 255, 255, 255, 255, 255, 255, 255},
              {253, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 251, 253, 255, 255, 255, 255, 255, 255, 255, 255},
              {252, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {249, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 255, 253, 255, 255, 255, 255, 255, 255, 255, 255},
              {250, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
            },
        {
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
              {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
            }
      }
};

/* section 17.2 of spec */
static const uint8_t vp8_mv_update_prob[2][19] = {
  {
        237,
        246,
        253, 253, 254, 254, 254, 254, 254,
      254, 254, 254, 254, 254, 250, 250, 252, 254, 254},
  {
        231,
        243,
        245, 253, 254, 254, 254, 254, 254,
      254, 254, 254, 254, 254, 251, 251, 254, 254, 254}
};

/* section 13.4 of spec */
static const uint8_t default_coef_probs[4][8][3][11] = {
  {
        {
              {128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
              {128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
              {128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128}
            },
        {
              {253, 136, 254, 255, 228, 219, 128, 128, 128, 128, 128},
              {189, 129, 242, 255, 227, 213, 255, 219, 128, 128, 128},
              {106, 126, 227, 252, 214, 209, 255, 255, 128, 128, 128}
            },
        {
              {1, 98, 248, 255, 236, 226, 255, 255, 128, 128, 128},
              {181, 133, 238, 254, 221, 234, 255, 154, 128, 128, 128},
              {78, 134, 202, 247, 198, 180, 255, 219, 128, 128, 128}
            },
        {
              {1, 185, 249, 255, 243, 255, 128, 128, 128, 128, 128},
              {184, 150, 247, 255, 236, 224, 128, 128, 128, 128, 128},
              {77, 110, 216, 255, 236, 230, 128, 128, 128, 128, 128}
            },
        {
              {1, 101, 251, 255, 241, 255, 128, 128, 128, 128, 128},
              {170, 139, 241, 252, 236, 209, 255, 255, 128, 128, 128},
              {37, 116, 196, 243, 228, 255, 255, 255, 128, 128, 128}
            },
        {
              {1, 204, 254, 255, 245, 255, 128, 128, 128, 128, 128},
              {207, 160, 250, 255, 238, 128, 128, 128, 128, 128, 128},
              {102, 103, 231, 255, 211, 171, 128, 128, 128, 128, 128}
            },
        {
              {1, 152, 252, 255, 240, 255, 128, 128, 128, 128, 128},
              {177, 135, 243, 255, 234, 225, 128, 128, 128, 128, 128},
              {80, 129, 211, 255, 194, 224, 128, 128, 128, 128, 128}
            },
        {
              {1, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128},
              {246, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128},
              {255, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128}
            }
      },
  {
        {
              {198, 35, 237, 223, 193, 187, 162, 160, 145, 155, 62},
              {131, 45, 198, 221, 172, 176, 220, 157, 252, 221, 1},
              {68, 47, 146, 208, 149, 167, 221, 162, 255, 223, 128}
            },
        {
              {1, 149, 241, 255, 221, 224, 255, 255, 128, 128, 128},
              {184, 141, 234, 253, 222, 220, 255, 199, 128, 128, 128},
              {81, 99, 181, 242, 176, 190, 249, 202, 255, 255, 128}
            },
        {
              {1, 129, 232, 253, 214, 197, 242, 196, 255, 255, 128},
              {99, 121, 210, 250, 201, 198, 255, 202, 128, 128, 128},
              {23, 91, 163, 242, 170, 187, 247, 210, 255, 255, 128}
            },
        {
              {1, 200, 246, 255, 234, 255, 128, 128, 128, 128, 128},
              {109, 178, 241, 255, 231, 245, 255, 255, 128, 128, 128},
              {44, 130, 201, 253, 205, 192, 255, 255, 128, 128, 128}
            },
        {
              {1, 132, 239, 251, 219, 209, 255, 165, 128, 128, 128},
              {94, 136, 225, 251, 218, 190, 255, 255, 128, 128, 128},
              {22, 100, 174, 245, 186, 161, 255, 199, 128, 128, 128}
            },
        {
              {1, 182, 249, 255, 232, 235, 128, 128, 128, 128, 128},
              {124, 143, 241, 255, 227, 234, 128, 128, 128, 128, 128},
              {35, 77, 181, 251, 193, 211, 255, 205, 128, 128, 128}
            },
        {
              {1, 157, 247, 255, 236, 231, 255, 255, 128, 128, 128},
              {121, 141, 235, 255, 225, 227, 255, 255, 128, 128, 128},
              {45, 99, 188, 251, 195, 217, 255, 224, 128, 128, 128}
            },
        {
              {1, 1, 251, 255, 213, 255, 128, 128, 128, 128, 128},
              {203, 1, 248, 255, 255, 128, 128, 128, 128, 128, 128},
              {137, 1, 177, 255, 224, 255, 128, 128, 128, 128, 128}
            }
      },
  {
        {
              {253, 9, 248, 251, 207, 208, 255, 192, 128, 128, 128},
              {175, 13, 224, 243, 193, 185, 249, 198, 255, 255, 128},
              {73, 17, 171, 221, 161, 179, 236, 167, 255, 234, 128}
            },
        {
              {1, 95, 247, 253, 212, 183, 255, 255, 128, 128, 128},
              {239, 90, 244, 250, 211, 209, 255, 255, 128, 128, 128},
              {155, 77, 195, 248, 188, 195, 255, 255, 128, 128, 128}
            },
        {
              {1, 24, 239, 251, 218, 219, 255, 205, 128, 128, 128},
              {201, 51, 219, 255, 196, 186, 128, 128, 128, 128, 128},
              {69, 46, 190, 239, 201, 218, 255, 228, 128, 128, 128}
            },
        {
              {1, 191, 251, 255, 255, 128, 128, 128, 128, 128, 128},
              {223, 165, 249, 255, 213, 255, 128, 128, 128, 128, 128},
              {141, 124, 248, 255, 255, 128, 128, 128, 128, 128, 128}
            },
        {
              {1, 16, 248, 255, 255, 128, 128, 128, 128, 128, 128},
              {190, 36, 230, 255, 236, 255, 128, 128, 128, 128, 128},
              {149, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128}
            },
        {
              {1, 226, 255, 128, 128, 128, 128, 128, 128, 128, 128},
              {247, 192, 255, 128, 128, 128, 128, 128, 128, 128, 128},
              {240, 128, 255, 128, 128, 128, 128, 128, 128, 128, 128}
            },
        {
              {1, 134, 252, 255, 255, 128, 128, 128, 128, 128, 128},
              {213, 62, 250, 255, 255, 128, 128, 128, 128, 128, 128},
              {55, 93, 255, 128, 128, 128, 128, 128, 128, 128, 128}
            },
        {
              {128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
              {128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
              {128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128}
            }
      },
  {
        {
              {202, 24, 213, 235, 186, 191, 220, 160, 240, 175, 255},
              {126, 38, 182, 232, 169, 184, 228, 174, 255, 187, 128},
              {61, 46, 138, 219, 151, 178, 240, 170, 255, 216, 128}
            },
        {
              {1, 112, 230, 250, 199, 191, 247, 159, 255, 255, 128},
              {166, 109, 228, 252, 211, 215, 255, 174, 128, 128, 128},
              {39, 77, 162, 232, 172, 180, 245, 178, 255, 255, 128}
            },
        {
              {1, 52, 220, 246, 198, 199, 249, 220, 255, 255, 128},
              {124, 74, 191, 243, 183, 193, 250, 221, 255, 255, 128},
              {24, 71, 130, 219, 154, 170, 243, 182, 255, 255, 128}
            },
        {
              {1, 182, 225, 249, 219, 240, 255, 224, 128, 128, 128},
              {149, 150, 226, 252, 216, 205, 255, 171, 128, 128, 128},
              {28, 108, 170, 242, 183, 194, 254, 223, 255, 255, 128}
            },
        {
              {1, 81, 230, 252, 204, 203, 255, 192, 128, 128, 128},
              {123, 102, 209, 247, 188, 196, 255, 233, 128, 128, 128},
              {20, 95, 153, 243, 164, 173, 255, 203, 128, 128, 128}
            },
        {
              {1, 222, 248, 255, 216, 213, 128, 128, 128, 128, 128},
              {168, 175, 246, 252, 235, 205, 255, 255, 128, 128, 128},
              {47, 116, 215, 255, 211, 212, 255, 255, 128, 128, 128}
            },
        {
              {1, 121, 236, 253, 212, 214, 255, 255, 128, 128, 128},
              {141, 84, 213, 252, 201, 202, 255, 219, 128, 128, 128},
              {42, 80, 160, 240, 162, 185, 255, 205, 128, 128, 128}
            },
        {
              {1, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128},
              {244, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128},
              {238, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128}
            }
      }
};

/* section 17.2 of spec */
static const uint8_t vp8_default_mv_context[2][19] = {
  {
        162,
        128,
        225, 146, 172, 147, 214, 39, 156,
      128, 129, 132, 75, 145, 178, 206, 239, 254, 254},
  {
        164,
        128,
        204, 170, 119, 235, 140, 230, 228,
      128, 130, 130, 74, 148, 180, 203, 236, 254, 254}
};


#define READ_BIT(br, val, field_name)  val = vp8_decode_value(br, 1)
#define READ_N_BITS(br, val, nbits, field_name)  val = vp8_decode_value(br, nbits)

/********** API **********/

/**
 * vp8_bool_decoder_debug_status:
 * @br: the #BOOL_DECODER to read
 *
 * helper function to track range decoder status, debug purpose
 *
 */
static void
vp8_bool_decoder_debug_status (BOOL_DECODER * br, const uint8_t * buf_start)
{
  LOG_DEBUG
      ("BOOL_DECODER: %x bytes read with %2d bits not parsed yet, code_word is:%8x, range-high is %2x\n",
      (uint32_t) (br->user_buffer - buf_start), 8 + br->count,
      (uint64_t) br->value, br->range);
  LOG_DEBUG ("user_buffer: %p, user_buffer_start: %p\n", br->user_buffer,
      buf_start);
}


static bool
update_segmentation (BOOL_DECODER * bool_decoder,
    Vp8FrameHdr * frame_hdr)
{
  Vp8Segmentation *seg = &frame_hdr->multi_frame_data->segmentation;
  int i, tmp;

  READ_BIT (bool_decoder, seg->segmentation_enabled, "segmentation_enabled");
  if (!seg->segmentation_enabled) {
    return true;
  }

  READ_BIT (bool_decoder, seg->update_mb_segmentation_map,
      "update_mb_segmentation_map");
  READ_BIT (bool_decoder, seg->update_segment_feature_data,
      "update_segment_feature_data");
  if (seg->update_segment_feature_data) {
    READ_BIT (bool_decoder, seg->segment_feature_mode, "segment_feature_mode");
    for (i = 0; i < 4; i++) {
      READ_BIT (bool_decoder, tmp, "quantizer_update");
      if (tmp) {
        READ_N_BITS (bool_decoder, seg->quantizer_update_value[i], 7,
            "quantizer_update_value");
        READ_BIT (bool_decoder, tmp, "quantizer_update_sign");
        if (tmp)
          seg->quantizer_update_value[i] = -seg->quantizer_update_value[i];
      }
    }

    for (i = 0; i < 4; i++) {
      READ_BIT (bool_decoder, tmp, "loop_filter_update");
      if (tmp) {
        READ_N_BITS (bool_decoder, seg->lf_update_value[i], 6,
            "lf_update_value");
        READ_BIT (bool_decoder, tmp, "lf_update_sign");
        if (tmp)
          seg->lf_update_value[i] = -seg->lf_update_value[i];
      }
    }
  }

  if (seg->update_mb_segmentation_map) {
    memset (seg->segment_prob, 0xff, 3);
    for (i = 0; i < 3; i++) {
      READ_BIT (bool_decoder, tmp, "segment_prob_update");
      if (tmp) {
        READ_N_BITS (bool_decoder, seg->segment_prob[i], 8, "segment_prob");
      }
    }
  }

  return true;
}

static bool
mb_lf_adjustments (BOOL_DECODER * bool_decoder,
    Vp8FrameHdr * frame_hdr)
{
  Vp8MbLfAdjustments *mb_lf_adjust =
      &frame_hdr->multi_frame_data->mb_lf_adjust;
  int i, tmp;

  READ_BIT (bool_decoder, mb_lf_adjust->loop_filter_adj_enable,
      "loop_filter_adj_enable");
  if (mb_lf_adjust->loop_filter_adj_enable) {
    READ_BIT (bool_decoder, mb_lf_adjust->mode_ref_lf_delta_update,
        "mode_ref_lf_delta_update");
    if (mb_lf_adjust->mode_ref_lf_delta_update) {
      for (i = 0; i < 4; i++) {
        READ_BIT (bool_decoder, tmp, "ref_frame_delta_update_flag");
        if (tmp) {
          READ_N_BITS (bool_decoder, mb_lf_adjust->ref_frame_delta[i], 6,
              "delta_magnitude");
          READ_BIT (bool_decoder, tmp, "delta_sign");
          if (tmp)
            mb_lf_adjust->ref_frame_delta[i] =
                -mb_lf_adjust->ref_frame_delta[i];
        }
      }

      for (i = 0; i < 4; i++) {
        READ_BIT (bool_decoder, tmp, "mb_mode_delta_update_flag");
        if (tmp) {
          READ_N_BITS (bool_decoder, mb_lf_adjust->mb_mode_delta[i], 6,
              "delta_magnitude");
          READ_BIT (bool_decoder, tmp, "delta_sign");
          if (tmp)
            mb_lf_adjust->mb_mode_delta[i] = -mb_lf_adjust->mb_mode_delta[i];
        }
      }
    }
  }

  return true;
}

static bool
quant_indices_parse (BOOL_DECODER * bool_decoder,
    Vp8FrameHdr * frame_hdr)
{
  Vp8QuantIndices *quant_indices = &frame_hdr->quant_indices;
  int tmp;

  memset (quant_indices, 0, sizeof (*quant_indices));
  READ_N_BITS (bool_decoder, quant_indices->y_ac_qi, 7, "y_ac_qi");

  READ_BIT (bool_decoder, tmp, "y_dc_delta_present");
  if (tmp) {
    READ_N_BITS (bool_decoder, quant_indices->y_dc_delta, 4,
        "y_dc_delta_present");
    READ_BIT (bool_decoder, tmp, "y_dc_delta_sign");
    if (tmp)
      quant_indices->y_dc_delta = -quant_indices->y_dc_delta;
  }

  READ_BIT (bool_decoder, tmp, "y2_dc_delta_present");
  if (tmp) {
    READ_N_BITS (bool_decoder, quant_indices->y2_dc_delta, 4,
        "y2_dc_delta_magnitude");
    READ_BIT (bool_decoder, tmp, "y2_dc_delta_sign");
    if (tmp)
      quant_indices->y2_dc_delta = -quant_indices->y2_dc_delta;
  }

  READ_BIT (bool_decoder, tmp, "y2_ac_delta_present");
  if (tmp) {
    READ_N_BITS (bool_decoder, quant_indices->y2_ac_delta, 4,
        "y2_ac_delta_magnitude");
    READ_BIT (bool_decoder, tmp, "y2_ac_delta_sign");
    if (tmp)
      quant_indices->y2_ac_delta = -quant_indices->y2_ac_delta;
  }

  READ_BIT (bool_decoder, tmp, "uv_dc_delta_present");
  if (tmp) {
    READ_N_BITS (bool_decoder, quant_indices->uv_dc_delta, 4,
        "uv_dc_delta_magnitude");
    READ_BIT (bool_decoder, tmp, "uv_dc_delta_sign");
    if (tmp)
      quant_indices->uv_dc_delta = -quant_indices->uv_dc_delta;
  }

  READ_BIT (bool_decoder, tmp, "uv_ac_delta_present");
  if (tmp) {
    READ_N_BITS (bool_decoder, quant_indices->uv_ac_delta, 4,
        "uv_ac_delta_magnitude");
    READ_BIT (bool_decoder, tmp, "uv_ac_delta_sign");
    if (tmp)
      quant_indices->uv_ac_delta = -quant_indices->uv_ac_delta;
  }
  return true;
}

static bool
token_prob_update (BOOL_DECODER * bool_decoder,
    Vp8FrameHdr * frame_hdr)
{
  Vp8TokenProbUpdate *token_prob_update =
      &frame_hdr->multi_frame_data->token_prob_update;
  int i, j, k, l;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 8; j++) {
      for (k = 0; k < 3; k++) {
        for (l = 0; l < 11; l++) {
          if (vp8dx_decode_bool (bool_decoder,
                  vp8_token_update_probs[i][j][k][l])) {
            READ_N_BITS (bool_decoder,
                token_prob_update->coeff_prob[i][j][k][l], 8,
                "token_prob_update");
            LOG_DEBUG ("        coeff_prob[%d][%d][%d][%d]: %d\n", i, j, k, l,
                token_prob_update->coeff_prob[i][j][k][l]);
          }
        }
      }
    }
  }

  return true;
}

static bool
mv_prob_update (BOOL_DECODER * bool_decoder, Vp8FrameHdr * frame_hdr)
{
  Vp8MvProbUpdate *mv_prob_update =
      &frame_hdr->multi_frame_data->mv_prob_update;
  int i, j, x;

  for (i = 0; i < 2; i++) {
    for (j = 0; j < 19; j++) {
      if (vp8dx_decode_bool (bool_decoder, vp8_mv_update_prob[i][j])) {
        READ_N_BITS (bool_decoder, x, 7, "mv_prob_update");
        mv_prob_update->prob[i][j] = x ? x << 1 : 1;
        LOG_DEBUG ("      mv_prob_update->prob[%d][%d]: %d\n", i, j,
            mv_prob_update->prob[i][j]);
      }
    }
  }
  return true;
}

/**
 * vp8_parse_frame_header:
 * @frame_hdr: The #Vp8FrameHdr to fill
 * @data: The data to parse
 * @offset: offset from which to start the parsing
 * @size: The size of the @data to parse
 *
 * Parses @data and fills @frame_hdr with the information
 *
 * VP8 parser depends on properly framed data since there is
 * no sync code in VP8 stream.
 * @frame->multi_frame_data should be set by caller, which constains
 * parameters inheriting from previous frame; parser may or may not
 * update them for current frame.
 *
 * Returns: a #Vp8ParseResult
 */
Vp8ParseResult
vp8_parse_frame_header (Vp8FrameHdr * frame_hdr, const uint8_t * data,
    uint32_t offset, uint32_t size)
{
  ByteReader byte_reader;
  BOOL_DECODER bool_decoder;
  Vp8RangeDecoderStatus *state = NULL;
  uint32_t frame_tag, tmp;
  uint16_t tmp_16;
  int pos, i;

  if (!frame_hdr->multi_frame_data) {
    LOG_WARNING ("multi_frame_data should be set by the caller of vp8 parser");
    goto error;
  }
  /* Uncompressed Data Chunk */
  byte_reader_init (&byte_reader, data, size);
  if (!byte_reader_get_uint24_le (&byte_reader, &frame_tag)) {
    goto error;
  }

  frame_hdr->key_frame = frame_tag & 0x01;
  frame_hdr->version = (frame_tag >> 1) & 0x07;
  frame_hdr->show_frame = (frame_tag >> 4) & 0x01;
  frame_hdr->first_part_size = (frame_tag >> 5) & 0x7ffff;

  if (frame_hdr->key_frame == VP8_KEY_FRAME) {
    if (!byte_reader_get_uint24_be (&byte_reader, &tmp)) {
      goto error;
    }
    if (tmp != 0x9d012a)
      LOG_WARNING ("vp8 parser: invalid start code in frame header.");

    if (!byte_reader_get_uint16_le (&byte_reader, &tmp_16)) {
      goto error;
    }
    frame_hdr->width = tmp_16 & 0x3fff;
    frame_hdr->horizontal_scale = tmp_16 >> 14;

    if (!byte_reader_get_uint16_le (&byte_reader, &tmp_16)) {
      goto error;
    }
    frame_hdr->height = tmp_16 & 0x3fff;
    frame_hdr->vertical_scale = (tmp_16 >> 14);

    vp8_parse_init_default_multi_frame_data (frame_hdr->multi_frame_data);
  }
  /* Frame Header */
  pos = byte_reader_get_pos (&byte_reader);
  if (vp8dx_start_decode (&bool_decoder, data + pos, size - pos))
    goto error;

  if (frame_hdr->key_frame == VP8_KEY_FRAME) {
    READ_BIT (&bool_decoder, frame_hdr->color_space, "color_space");
    READ_BIT (&bool_decoder, frame_hdr->clamping_type, "clamping_type");
  }

  if (!update_segmentation (&bool_decoder, frame_hdr)) {
    goto error;
  }

  READ_BIT (&bool_decoder, frame_hdr->filter_type, "filter_type");
  READ_N_BITS (&bool_decoder, frame_hdr->loop_filter_level, 6,
      "loop_filter_level");
  READ_N_BITS (&bool_decoder, frame_hdr->sharpness_level, 3,
      "sharpness_level");

  if (!mb_lf_adjustments (&bool_decoder, frame_hdr)) {
    goto error;
  }

  READ_N_BITS (&bool_decoder, frame_hdr->log2_nbr_of_dct_partitions, 2,
      "log2_nbr_of_dct_partitions");
  if (frame_hdr->log2_nbr_of_dct_partitions) {
    const uint8_t *part_size_ptr = data + frame_hdr->first_part_size;
    int par_count = 1 << frame_hdr->log2_nbr_of_dct_partitions;
    int i = 0;

    if (frame_hdr->key_frame == VP8_KEY_FRAME)
      part_size_ptr += VP8_UNCOMPRESSED_DATA_SIZE_KEY_FRAME;
    else
      part_size_ptr += VP8_UNCOMPRESSED_DATA_SIZE_NON_KEY_FRAME;
    /* the last partition size is not specified (see spec page 9) */
    for (i = 0; i < par_count - 1; i++) {
      uint8_t c[3];
      c[0] = *part_size_ptr;
      c[1] = *(part_size_ptr + 1);
      c[2] = *(part_size_ptr + 2);
      frame_hdr->partition_size[i] = c[0] + (c[1] << 8) + (c[2] << 16);
      part_size_ptr += 3;
    }
  }

  if (!quant_indices_parse (&bool_decoder, frame_hdr)) {
    goto error;
  }

  if (frame_hdr->key_frame == VP8_KEY_FRAME) {
    READ_BIT (&bool_decoder, frame_hdr->refresh_entropy_probs,
        "refresh_entropy_probs");
  } else {
    READ_BIT (&bool_decoder, frame_hdr->refresh_golden_frame,
        "refresh_golden_frame");
    READ_BIT (&bool_decoder, frame_hdr->refresh_alternate_frame,
        "refresh_alternate_frame");

    if (!frame_hdr->refresh_golden_frame) {
      READ_N_BITS (&bool_decoder, frame_hdr->copy_buffer_to_golden, 2,
          "copy_buffer_to_golden");
    }

    if (!frame_hdr->refresh_alternate_frame) {
      READ_N_BITS (&bool_decoder, frame_hdr->copy_buffer_to_alternate, 2,
          "copy_buffer_to_alternate");
    }

    READ_BIT (&bool_decoder, frame_hdr->sign_bias_golden, "sign_bias_golden");
    READ_BIT (&bool_decoder, frame_hdr->sign_bias_alternate,
        "sign_bias_alternate");
    READ_BIT (&bool_decoder, frame_hdr->refresh_entropy_probs,
        "refresh_entropy_probs");
    READ_BIT (&bool_decoder, frame_hdr->refresh_last, "refresh_last");
  }

  if (!token_prob_update (&bool_decoder, frame_hdr)) {
    goto error;
  }

  READ_BIT (&bool_decoder, frame_hdr->mb_no_skip_coeff, "mb_no_skip_coeff");
  if (frame_hdr->mb_no_skip_coeff) {
    READ_N_BITS (&bool_decoder, frame_hdr->prob_skip_false, 8,
        "prob_skip_false");
  }

  if (frame_hdr->key_frame != VP8_KEY_FRAME) {
    READ_N_BITS (&bool_decoder, frame_hdr->prob_intra, 8, "prob_intra");
    READ_N_BITS (&bool_decoder, frame_hdr->prob_last, 8, "prob_last");
    READ_N_BITS (&bool_decoder, frame_hdr->prob_gf, 8, "prob_gf");

    READ_BIT (&bool_decoder, frame_hdr->intra_16x16_prob_update_flag,
        "intra_16x16_prob_update_flag");

    if (frame_hdr->intra_16x16_prob_update_flag) {
      for (i = 0; i < 4; i++) {
        READ_N_BITS (&bool_decoder, frame_hdr->intra_16x16_prob[i], 8,
            "intra_16x16_prob");
      }
    }

    READ_BIT (&bool_decoder, frame_hdr->intra_chroma_prob_update_flag,
        "intra_chroma_prob_update_flag");
    if (frame_hdr->intra_chroma_prob_update_flag) {
      for (i = 0; i < 3; i++) {
        READ_N_BITS (&bool_decoder, frame_hdr->intra_chroma_prob[i], 8,
            "intra_chroma_prob");
      }
    }

    if (!mv_prob_update (&bool_decoder, frame_hdr)) {
      goto error;
    }

  }

  vp8_bool_decoder_debug_status (&bool_decoder, data + pos);

  state = &frame_hdr->rangedecoder_state;
  if (bool_decoder.count < 0)
    vp8dx_bool_decoder_fill (&bool_decoder);

  state->range = bool_decoder.range;
  state->code_word = (uint8_t) ((bool_decoder.value) >> (VP8_BD_VALUE_SIZE - 8));
  state->remaining_bits = 8 + bool_decoder.count;
  state->buffer = bool_decoder.user_buffer;

  while (state->remaining_bits >= 8) {
    state->buffer--;
    state->remaining_bits -= 8;
  }

  return VP8_PARSER_OK;

error:
  LOG_WARNING ("failed in parsing VP8 frame header");
  return VP8_PARSER_ERROR;
}

/**
 * vp8_parse_init_default_multi_frame_data:
 * @multi_frame_data: The #Vp8MultiFrameData to init
 *
 * init @multi_frame_data with default value
 *
 * decoder uses it to se default parameter, in case key frame
 * updates probability table but refresh_entropy_probs isn't set
 *
 * Returns: true if successful, false otherwise.
 */
bool
vp8_parse_init_default_multi_frame_data (Vp8MultiFrameData *
    multi_frame_data)
{
  Vp8TokenProbUpdate *token_prob_update;
  Vp8MvProbUpdate *mv_prob_update;
  Vp8MbLfAdjustments *mb_lf_adjust;

  if (!multi_frame_data)
    return false;

  token_prob_update = &multi_frame_data->token_prob_update;
  mv_prob_update = &multi_frame_data->mv_prob_update;
  mb_lf_adjust = &multi_frame_data->mb_lf_adjust;

  if (!token_prob_update || !mv_prob_update || !mb_lf_adjust)
    return false;

  memcpy (token_prob_update->coeff_prob, default_coef_probs,
      sizeof (default_coef_probs));
  memset (mb_lf_adjust, 0, sizeof (*mb_lf_adjust));
  memcpy (mv_prob_update->prob, vp8_default_mv_context,
      sizeof (vp8_default_mv_context));

  return true;
}
