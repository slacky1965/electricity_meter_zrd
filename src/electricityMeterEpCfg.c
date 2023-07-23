/********************************************************************************************************
 * @file    electricityMeterEpCfg.c
 *
 * @brief   This is the source file for electricityMeterEpCfg
 *
 * @author  Zigbee Group
 * @date    2021
 *
 * @par     Copyright (c) 2021, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *			All rights reserved.
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/

/**********************************************************************
 * INCLUDES
 */
#include "tl_common.h"
#include "zcl_include.h"

#include "app_ui.h"
#include "electricityMeter.h"


/**********************************************************************
 * LOCAL CONSTANTS
 */
#ifndef ZCL_BASIC_MFG_NAME
#define ZCL_BASIC_MFG_NAME     		{6,'T','E','L','I','N','K'}
#endif
#ifndef ZCL_BASIC_MODEL_ID
#define ZCL_BASIC_MODEL_ID	   		{8,'T','L','S','R','8','2','x','x'}
#endif
#ifndef ZCL_BASIC_SW_BUILD_ID
#define ZCL_BASIC_SW_BUILD_ID     	{10,'0','1','2','2','0','5','2','0','1','7'}
#endif


/**********************************************************************
 * TYPEDEFS
 */


/**********************************************************************
 * GLOBAL VARIABLES
 */
/**
 *  @brief Definition for Incoming cluster / Sever Cluster
 */
const u16 electricityMeter_inClusterList[] =
{
	ZCL_CLUSTER_GEN_BASIC,
	ZCL_CLUSTER_GEN_IDENTIFY,
#ifdef ZCL_POLL_CTRL
    ZCL_CLUSTER_GEN_POLL_CONTROL,
#endif
    ZCL_CLUSTER_SE_METERING,
};

/**
 *  @brief Definition for Outgoing cluster / Client Cluster
 */
const u16 electricityMeter_outClusterList[] =
{
    ZCL_CLUSTER_GEN_BASIC,
#ifdef ZCL_GROUP
    ZCL_CLUSTER_GEN_GROUPS,
#endif
#ifdef ZCL_SCENE
    ZCL_CLUSTER_GEN_SCENES,
#endif
#ifdef ZCL_ON_OFF
    ZCL_CLUSTER_GEN_ON_OFF,
#endif
#ifdef ZCL_LEVEL_CTRL
    ZCL_CLUSTER_GEN_LEVEL_CONTROL,
#endif
#ifdef ZCL_OTA
    ZCL_CLUSTER_OTA,
#endif
#ifdef ZCL_ZLL_COMMISSIONING
    ZCL_CLUSTER_TOUCHLINK_COMMISSIONING,
#endif
    ZCL_CLUSTER_SE_METERING,
    ZCL_CLUSTER_GEN_TIME,
    ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT,
};

/**
 *  @brief Definition for Server cluster number and Client cluster number
 */
#define ELECTRICITY_METER_IN_CLUSTER_NUM		(sizeof(electricityMeter_inClusterList)/sizeof(electricityMeter_inClusterList[0]))
#define ELECTRICITY_METER_OUT_CLUSTER_NUM		(sizeof(electricityMeter_outClusterList)/sizeof(electricityMeter_outClusterList[0]))

/**
 *  @brief Definition for simple description for HA profile
 */
const af_simple_descriptor_t electricityMeter_simpleDesc = {
    HA_PROFILE_ID,                          /* Application profile identifier */
    HA_DEV_METER_INTERFACE,                 /* Application device identifier */
    ELECTRICITY_METER_EP1,                  /* Endpoint */
    1,                                      /* Application device version */
    0,                                      /* Reserved */
    ELECTRICITY_METER_IN_CLUSTER_NUM,       /* Application input cluster count */
    ELECTRICITY_METER_OUT_CLUSTER_NUM,      /* Application output cluster count */
    (u16*) electricityMeter_inClusterList,  /* Application input cluster list */
    (u16*) electricityMeter_outClusterList, /* Application output cluster list */
};



/* Basic */
zcl_basicAttr_t g_zcl_basicAttrs =
{
	.zclVersion 	= 0x03,
	.appVersion 	= APP_RELEASE,
	.stackVersion 	= (STACK_RELEASE|STACK_BUILD),
	.hwVersion		= 0x00,
	.manuName		= ZCL_BASIC_MFG_NAME,
	.modelId		= ZCL_BASIC_MODEL_ID,
    .dateCode       = ZCL_BASIC_DATE_CODE,
	.powerSource	= POWER_SOURCE_MAINS_1_PHASE,
	.swBuildId		= ZCL_BASIC_SW_BUILD_ID,
	.deviceEnable	= TRUE,
};

u8 zclVersionServer;

const zclAttrInfo_t basic_attrTbl[] =
{
	{ ZCL_ATTRID_BASIC_ZCL_VER,      		ZCL_DATA_TYPE_UINT8,    ACCESS_CONTROL_READ,  						(u8*)&g_zcl_basicAttrs.zclVersion},
	{ ZCL_ATTRID_BASIC_APP_VER,      		ZCL_DATA_TYPE_UINT8,    ACCESS_CONTROL_READ,  						(u8*)&g_zcl_basicAttrs.appVersion},
	{ ZCL_ATTRID_BASIC_STACK_VER,    		ZCL_DATA_TYPE_UINT8,    ACCESS_CONTROL_READ,  						(u8*)&g_zcl_basicAttrs.stackVersion},
	{ ZCL_ATTRID_BASIC_HW_VER,       		ZCL_DATA_TYPE_UINT8,    ACCESS_CONTROL_READ,  						(u8*)&g_zcl_basicAttrs.hwVersion},
	{ ZCL_ATTRID_BASIC_MFR_NAME,     		ZCL_DATA_TYPE_CHAR_STR, ACCESS_CONTROL_READ,  						(u8*)g_zcl_basicAttrs.manuName},
	{ ZCL_ATTRID_BASIC_MODEL_ID,     		ZCL_DATA_TYPE_CHAR_STR, ACCESS_CONTROL_READ,  						(u8*)g_zcl_basicAttrs.modelId},
	{ ZCL_ATTRID_BASIC_POWER_SOURCE, 		ZCL_DATA_TYPE_ENUM8,    ACCESS_CONTROL_READ,  						(u8*)&g_zcl_basicAttrs.powerSource},
	{ ZCL_ATTRID_BASIC_DEV_ENABLED,  		ZCL_DATA_TYPE_BOOLEAN,  ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE, (u8*)&g_zcl_basicAttrs.deviceEnable},
	{ ZCL_ATTRID_BASIC_SW_BUILD_ID,  		ZCL_DATA_TYPE_CHAR_STR, ACCESS_CONTROL_READ,  						(u8*)&g_zcl_basicAttrs.swBuildId},

	{ ZCL_ATTRID_GLOBAL_CLUSTER_REVISION, 	ZCL_DATA_TYPE_UINT16,  	ACCESS_CONTROL_READ,  						(u8*)&zcl_attr_global_clusterRevision},
};

#define ZCL_BASIC_ATTR_NUM	  sizeof(basic_attrTbl) / sizeof(zclAttrInfo_t)

u8 zclVersionServer;

const zclAttrInfo_t version_attrTbl[] =
{
    { ZCL_ATTRID_BASIC_ZCL_VER,             ZCL_DATA_TYPE_UINT8,    ACCESS_CONTROL_READ,    (u8*)&zclVersionServer},

    { ZCL_ATTRID_GLOBAL_CLUSTER_REVISION,   ZCL_DATA_TYPE_UINT16,   ACCESS_CONTROL_READ,    (u8*)&zcl_attr_global_clusterRevision},
};

#define ZCL_BASIC_SERVER_ATTR_NUM    sizeof(version_attrTbl) / sizeof(zclAttrInfo_t)


/* Identify */
zcl_identifyAttr_t g_zcl_identifyAttrs =
{
	.identifyTime	= 0x0000,
};

const zclAttrInfo_t identify_attrTbl[] =
{
	{ ZCL_ATTRID_IDENTIFY_TIME,  			ZCL_DATA_TYPE_UINT16,   ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE, (u8*)&g_zcl_identifyAttrs.identifyTime },

	{ ZCL_ATTRID_GLOBAL_CLUSTER_REVISION, 	ZCL_DATA_TYPE_UINT16,  	ACCESS_CONTROL_READ,  						(u8*)&zcl_attr_global_clusterRevision},
};

#define ZCL_IDENTIFY_ATTR_NUM	 sizeof(identify_attrTbl) / sizeof(zclAttrInfo_t)

zcl_timeAttr_t g_zcl_timeAttrs = {
    .time_utc = 0xffffffff,
};

const zclAttrInfo_t time_attrTbl[] = {
    { ZCL_ATTRID_TIME, ZCL_DATA_TYPE_UTC,   ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE, (u8*)&g_zcl_timeAttrs.time_utc },

    { ZCL_ATTRID_GLOBAL_CLUSTER_REVISION,   ZCL_DATA_TYPE_UINT16,   ACCESS_CONTROL_READ, (u8*)&zcl_attr_global_clusterRevision},
};

#define ZCL_TIME_ATTR_NUM    sizeof(time_attrTbl) / sizeof(zclAttrInfo_t)

zcl_seAttr_t g_zcl_seAttrs = {
    .tariff_1 = 0,
    .tariff_2 = 0,
    .tariff_3 = 0,
    .tariff_4 = 0,
    .unit_of_measure = 0x00,    // kWh
    .multiplier = 0,
    .divisor = 0,
    .summation_formatting = 0,
    .battery_percentage = 100,
    .serial_number = {8,'1','1','1','1','1','1','1','1'},
    .device_type = 0,           // Electric Metering
};

const zclAttrInfo_t se_attrTbl[] = {
    {ZCL_ATTRID_CURRENT_TIER_1_SUMMATION_DELIVERD,  ZCL_DATA_TYPE_UINT48,       ACCESS_CONTROL_READ | ACCESS_CONTROL_REPORTABLE,    (u8*)&g_zcl_seAttrs.tariff_1},
    {ZCL_ATTRID_CURRENT_TIER_2_SUMMATION_DELIVERD,  ZCL_DATA_TYPE_UINT48,       ACCESS_CONTROL_READ | ACCESS_CONTROL_REPORTABLE,    (u8*)&g_zcl_seAttrs.tariff_2},
    {ZCL_ATTRID_CURRENT_TIER_3_SUMMATION_DELIVERD,  ZCL_DATA_TYPE_UINT48,       ACCESS_CONTROL_READ | ACCESS_CONTROL_REPORTABLE,    (u8*)&g_zcl_seAttrs.tariff_3},
    {ZCL_ATTRID_CURRENT_TIER_4_SUMMATION_DELIVERD,  ZCL_DATA_TYPE_UINT48,       ACCESS_CONTROL_READ | ACCESS_CONTROL_REPORTABLE,    (u8*)&g_zcl_seAttrs.tariff_4},
    {ZCL_ATTRID_UNIT_OF_MEASURE,                    ZCL_DATA_TYPE_UINT8,        ACCESS_CONTROL_READ,                                (u8*)&g_zcl_seAttrs.unit_of_measure},
    {ZCL_ATTRID_MULTIPLIER,                         ZCL_DATA_TYPE_UINT24,       ACCESS_CONTROL_READ,                                (u8*)&g_zcl_seAttrs.multiplier},
    {ZCL_ATTRID_DIVISOR,                            ZCL_DATA_TYPE_UINT24,       ACCESS_CONTROL_READ,                                (u8*)&g_zcl_seAttrs.divisor},
    {ZCL_ATTRID_SUMMATION_FORMATTING,               ZCL_DATA_TYPE_BITMAP8,      ACCESS_CONTROL_READ,                                (u8*)&g_zcl_seAttrs.summation_formatting},
    {ZCL_ATTRID_REMAINING_BATTERY_LIFE,             ZCL_DATA_TYPE_UINT8,        ACCESS_CONTROL_READ | ACCESS_CONTROL_REPORTABLE,    (u8*)&g_zcl_seAttrs.battery_percentage},
    {ZCL_ATTRID_METER_SERIAL_NUMBER,                ZCL_DATA_TYPE_OCTET_STR,    ACCESS_CONTROL_READ,                                (u8*)&g_zcl_seAttrs.serial_number},
    {ZCL_ATTRID_METERING_DEVICE_TYPE,               ZCL_DATA_TYPE_BITMAP8,      ACCESS_CONTROL_READ,                                (u8*)&g_zcl_seAttrs.device_type},

    { ZCL_ATTRID_GLOBAL_CLUSTER_REVISION,           ZCL_DATA_TYPE_UINT16,       ACCESS_CONTROL_READ,                                (u8*)&zcl_attr_global_clusterRevision},
};

#define ZCL_SE_ATTR_NUM    sizeof(se_attrTbl) / sizeof(zclAttrInfo_t)

zcl_msAttr_t g_zcl_msAttrs = {
    .type = 0x09,               // bit0: Active measurement (AC). bit3: Phase A measurement
    .current = 0xffff,
    .current_multiplier = 1,
    .current_divisor = 1,
    .voltage = 0xffff,
    .voltage_multiplier = 1,
    .voltage_divisor = 1,
};

const zclAttrInfo_t ms_attrTbl[] = {
    {ZCL_ATTRID_MEASUREMENT_TYPE,           ZCL_DATA_TYPE_BITMAP32, ACCESS_CONTROL_READ,                                (u8*)&g_zcl_msAttrs.type},
    {ZCL_ATTRID_LINE_CURRENT,               ZCL_DATA_TYPE_UINT16,   ACCESS_CONTROL_READ | ACCESS_CONTROL_REPORTABLE,    (u8*)&g_zcl_msAttrs.current},
    {ZCL_ATTRID_AC_CURRENT_MULTIPLIER,      ZCL_DATA_TYPE_UINT16,   ACCESS_CONTROL_READ | ACCESS_CONTROL_REPORTABLE,    (u8*)&g_zcl_msAttrs.current_multiplier},
    {ZCL_ATTRID_AC_CURRENT_DIVISOR,         ZCL_DATA_TYPE_UINT16,   ACCESS_CONTROL_READ | ACCESS_CONTROL_REPORTABLE,    (u8*)&g_zcl_msAttrs.current_divisor},
    {ZCL_ATTRID_RMS_VOLTAGE,                ZCL_DATA_TYPE_UINT16,   ACCESS_CONTROL_READ | ACCESS_CONTROL_REPORTABLE,    (u8*)&g_zcl_msAttrs.voltage},
    {ZCL_ATTRID_AC_VOLTAGE_MULTIPLIER,      ZCL_DATA_TYPE_UINT16,   ACCESS_CONTROL_READ | ACCESS_CONTROL_REPORTABLE,    (u8*)&g_zcl_msAttrs.voltage_multiplier},
    {ZCL_ATTRID_AC_CURRENT_DIVISOR,         ZCL_DATA_TYPE_UINT16,   ACCESS_CONTROL_READ | ACCESS_CONTROL_REPORTABLE,    (u8*)&g_zcl_msAttrs.voltage_divisor},

    { ZCL_ATTRID_GLOBAL_CLUSTER_REVISION,   ZCL_DATA_TYPE_UINT16,   ACCESS_CONTROL_READ,                                (u8*)&zcl_attr_global_clusterRevision},
};

#define ZCL_MS_ATTR_NUM    sizeof(ms_attrTbl) / sizeof(zclAttrInfo_t)
//ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT


#ifdef ZCL_GROUP
/* Group */
zcl_groupAttr_t g_zcl_groupAttrs =
{
	.nameSupport	= 0,
};

const zclAttrInfo_t group_attrTbl[] =
{
	{ ZCL_ATTRID_GROUP_NAME_SUPPORT,  		ZCL_DATA_TYPE_BITMAP8,  ACCESS_CONTROL_READ,  (u8*)&g_zcl_groupAttrs.nameSupport },

	{ ZCL_ATTRID_GLOBAL_CLUSTER_REVISION, 	ZCL_DATA_TYPE_UINT16,  	ACCESS_CONTROL_READ,  (u8*)&zcl_attr_global_clusterRevision},
};

#define ZCL_GROUP_ATTR_NUM	  sizeof(group_attrTbl) / sizeof(zclAttrInfo_t)
#endif

#ifdef ZCL_SCENE
/* Scene */
zcl_sceneAttr_t g_zcl_sceneAttrs =
{
	.sceneCount		= 0,
	.currentScene	= 0,
	.currentGroup	= 0x0000,
	.sceneValid		= FALSE,
	.nameSupport	= 0,
};

const zclAttrInfo_t scene_attrTbl[] =
{
    { ZCL_ATTRID_SCENE_SCENE_COUNT,       	ZCL_DATA_TYPE_UINT8,    ACCESS_CONTROL_READ,  (u8*)&g_zcl_sceneAttrs.sceneCount		},
    { ZCL_ATTRID_SCENE_CURRENT_SCENE,     	ZCL_DATA_TYPE_UINT8,    ACCESS_CONTROL_READ,  (u8*)&g_zcl_sceneAttrs.currentScene   },
    { ZCL_ATTRID_SCENE_CURRENT_GROUP,     	ZCL_DATA_TYPE_UINT16,   ACCESS_CONTROL_READ,  (u8*)&g_zcl_sceneAttrs.currentGroup   },
    { ZCL_ATTRID_SCENE_SCENE_VALID,       	ZCL_DATA_TYPE_BOOLEAN,  ACCESS_CONTROL_READ,  (u8*)&g_zcl_sceneAttrs.sceneValid     },
    { ZCL_ATTRID_SCENE_NAME_SUPPORT,      	ZCL_DATA_TYPE_BITMAP8,  ACCESS_CONTROL_READ,  (u8*)&g_zcl_sceneAttrs.nameSupport    },

    { ZCL_ATTRID_GLOBAL_CLUSTER_REVISION, 	ZCL_DATA_TYPE_UINT16,  	ACCESS_CONTROL_READ,  (u8*)&zcl_attr_global_clusterRevision},
};

#define ZCL_SCENE_ATTR_NUM	 sizeof(scene_attrTbl) / sizeof(zclAttrInfo_t)
#endif



/**
 *  @brief Definition for simple light ZCL specific cluster
 */
const zcl_specClusterInfo_t g_electricityMeterClusterList[] =
{
	{ZCL_CLUSTER_GEN_BASIC,     MANUFACTURER_CODE_NONE, ZCL_BASIC_ATTR_NUM, 	basic_attrTbl,  	zcl_basic_register,		electricityMeter_basicCb},
	{ZCL_CLUSTER_GEN_IDENTIFY,	MANUFACTURER_CODE_NONE, ZCL_IDENTIFY_ATTR_NUM,  identify_attrTbl,   zcl_identify_register,  electricityMeter_identifyCb},
#ifdef ZCL_GROUP
	{ZCL_CLUSTER_GEN_GROUPS,	MANUFACTURER_CODE_NONE, ZCL_GROUP_ATTR_NUM, 	group_attrTbl,  	zcl_group_register,		NULL},
#endif
#ifdef ZCL_SCENE
	{ZCL_CLUSTER_GEN_SCENES,	MANUFACTURER_CODE_NONE, ZCL_SCENE_ATTR_NUM,		scene_attrTbl,		zcl_scene_register,		electricityMeter_sceneCb},
#endif
	{ZCL_CLUSTER_GEN_TIME,      MANUFACTURER_CODE_NONE, ZCL_TIME_ATTR_NUM,      time_attrTbl,       zcl_time_register,      electricityMeter_timeCb},
    {ZCL_CLUSTER_SE_METERING,   MANUFACTURER_CODE_NONE, ZCL_SE_ATTR_NUM,        se_attrTbl,         zcl_metering_register,  electricityMeter_meteringCb},
    {ZCL_CLUSTER_MS_ELECTRICAL_MEASUREMENT,   MANUFACTURER_CODE_NONE, ZCL_MS_ATTR_NUM, ms_attrTbl,  zcl_electricalMeasure_register,  NULL},
};

u8 ELECTRICITY_METER_CB_CLUSTER_NUM = (sizeof(g_electricityMeterClusterList)/sizeof(g_electricityMeterClusterList[0]));


/**********************************************************************
 * FUNCTIONS
 */



