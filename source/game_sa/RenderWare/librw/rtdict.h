/*
* RenderWare API on top of librw (https://github.com/aap/librw).
* Started from the `fakerw` layer of re3/reVC (by aap), extended for the RenderWare 3.6 API that San Andreas uses.
* Only used when building with `NOTSA_LIBRW`.
*/
#ifndef __GTA_RTDICT_H__
#define __GTA_RTDICT_H__

#include "rwcore.h"

// TODO(librw): librw reads UV animation dictionaries itself (`rw::UVAnimDictionary`), there are no generic dictionaries
struct RtDict;
struct RtDictSchema;

RtDict *RtDictSchemaStreamReadDict(RtDictSchema *schema, RwStream *stream);
RtDict *RtDictSchemaSetCurrentDict(RtDictSchema *schema, RtDict *dict);
RtDict *RtDictSchemaGetCurrentDict(RtDictSchema *schema);
RwBool RtDictDestroy(RtDict *dictionary);

#endif // __GTA_RTDICT_H__
