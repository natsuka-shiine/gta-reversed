#pragma once

//! The original executable's sections are in `[NOTSA_IMAGE_BEGIN, NOTSA_IMAGE_END)`. Code first, data from `NOTSA_IMAGE_DATA_BEGIN` on.
#define NOTSA_IMAGE_BASE       0x400000
#define NOTSA_IMAGE_BEGIN      0x401000
#define NOTSA_IMAGE_DATA_BEGIN 0x858000
#define NOTSA_IMAGE_END        0xCB1000

//! Resource (`RT_RCDATA`) with the data: `uint32 begin, end`, followed by the bytes. See `tools/startup-data/make_image.py`
#define NOTSA_IMAGE_RESOURCE_NAME "NOTSA_IMAGE_DATA"

extern "C" unsigned char g_NotsaOriginalImage[NOTSA_IMAGE_END - NOTSA_IMAGE_BEGIN];
