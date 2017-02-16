/*
 *  Copyright 2011 Intel Corporation All Rights Reserved.
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

#ifndef JMESSAGE
#ifndef JERROR_LIBVA_EXT_H
/* First time through, define the enum list */
#define JMAKE_ENUM_LIST
#else
/* Repeated inclusions of this file are no-ops unless JMESSAGE is defined */
#define JMESSAGE(code,string)
#endif /* JERROR_LIBVA_EXT_H */
#endif /* JMESSAGE */

#ifdef JMAKE_ENUM_LIST

typedef enum {

#define JMESSAGE(code,string) code ,

#endif /* JMAKE_ENUM_LIST */

  JMESSAGE(JMSG_FIRSTLIBVAEXTCODE=124, "HW jpeg error start from here")

//VA API Extension
  JMESSAGE(JERR_JVA_INITIALIZE, "jva_initialize failed")
  JMESSAGE(JERR_JVA_DEINITIALIZE, "jva_deinitialize failed")
  JMESSAGE(JERR_JVA_CREATERESOURCE, "jva_create_resource failed")
  JMESSAGE(JERR_JVA_RELEASERESOURCE, "jva_release_resource failed")

  JMESSAGE(JERR_JVA_DECODE, "jva_decode failed")
  JMESSAGE(JERR_JVA_INVALID_PARAMS, "Invalid parameters pass to hw jpeg library")

  JMESSAGE(JERR_NULL_FUNC_POINTER, "NULL function pointer")

  JMESSAGE (JERR_RESOURCE_ALLOCATED, "Resource already allocated, can't request buffer anymore")
  JMESSAGE (JERR_BAD_FORMAT, "Resource already allocated, can't request buffer anymore")

  JMESSAGE (JERR_CINFO_NOT_FOUND, "Can't find according cinfo from context list")
  JMESSAGE (JERR_NULL_JDVA_CONTEXT, "The JD VA contex is invalid")

  JMESSAGE (JTRC_HW_PATH, "Video hardware is available, go to **HARDWARE** path")
  JMESSAGE (JTRC_SW_PATH, "Video hardware is busy or caps are not supported, go to *software* path")
//VA API Extension

#ifdef JMAKE_ENUM_LIST

  JMSG_LASTLIBVAEXTCODE
} J_MESSAGE_LIBVAEXT_CODE;

#undef JMAKE_ENUM_LIST
#endif /* JMAKE_ENUM_LIST */

/* Zap JMESSAGE macro so that future re-inclusions do nothing by default */
#undef JMESSAGE


#ifndef JERROR_LIBVA_EXT_H
#define JERROR_LIBVA_EXT_H

#endif /* JERROR_LIBVA_EXT_H */

