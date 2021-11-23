/*************************************************
Copyright,2006,LongTong co.,LTD
文件名：AFN0D.h
作者：TianYe
版本：0.9
完成日期:2006年8月9日
描述：AFN0D头文件
修改历史:
  01,06-8-9,Tianye created.
**************************************************/

#ifndef __INCafn0dH
#define __INCafc0dH

#include "common.h"

void AFN0D(INT8U *pDataHead, INT8U *pDataEnd,INT8U dataFrom, INT8U poll);

//组1
INT16U AFN0D001(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D002(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D003(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D004(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D005(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D006(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D007(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D008(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组2
INT16U AFN0D009(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D010(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D011(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D012(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组3
INT16U AFN0D017(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D018(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D019(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D020(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D021(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D022(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D023(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D024(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组4
INT16U AFN0D025(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D026(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D027(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D028(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D029(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D030(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D031(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D032(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组5
INT16U AFN0D033(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D034(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D035(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D036(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D037(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D038(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D039(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组6
INT16U AFN0D041(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D042(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D043(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D044(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D045(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D046(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组7
INT16U AFN0D049(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D050(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D051(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D052(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D053(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D054(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

INT16U AFN0D055(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);   //重庆规约
INT16U AFN0D056(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);   //重庆规约

//组8
INT16U AFN0D057(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D058(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D059(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D060(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D061(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D062(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组9
INT16U AFN0D065(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D066(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组10
INT16U AFN0D073(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D074(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D075(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D076(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//2017-7-18,Add,视在功率曲线
INT16U AFN0D077(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D078(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D079(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D080(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组11
INT16U AFN0D081(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D082(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D083(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D084(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D085(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D086(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D087(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D088(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组12
INT16U AFN0D089(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D090(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D091(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D092(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D093(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D094(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D095(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//2017-7-19,add,频率曲线
INT16U AFN0D096(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组13
INT16U AFN0D097(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D098(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D099(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D100(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D101(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D102(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D103(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D104(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组14
INT16U AFN0D105(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D106(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D107(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D108(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D109(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D110(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//LIGHTING
INT16U AFN0D111(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D112(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组15
INT16U AFN0D113(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D114(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D115(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D116(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D117(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D118(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组16
INT16U AFN0D121(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D122(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D123(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组17
INT16U AFN0D129(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D130(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组18
INT16U AFN0D138(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组19
INT16U AFN0D145(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D146(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D147(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D148(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

#ifdef LIGHTING
 INT16U AFN0D149(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
 INT16U AFN0D150(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
 INT16U AFN0D151(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
 INT16U AFN0D152(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
#endif

//组20
INT16U AFN0D153(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D154(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D155(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D156(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D157(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D158(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D159(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D160(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组21
INT16U AFN0D161(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D162(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D163(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D164(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D165(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D166(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D167(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D168(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组22
INT16U AFN0D169(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D170(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D171(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D172(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D173(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D174(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D175(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D176(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组23
INT16U AFN0D177(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D178(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D179(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D180(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D181(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D182(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D183(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D184(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组24
INT16U AFN0D185(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D186(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D187(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D188(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D189(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D190(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D191(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D192(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组25
INT16U AFN0D193(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D194(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D195(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D196(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组26
INT16U AFN0D201(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D202(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D203(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D204(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D205(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D206(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D207(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D208(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组27
INT16U AFN0D209(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D213(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D214(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D215(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D216(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

//组28
INT16U AFN0D217(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);
INT16U AFN0D218(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);

INT16U AFN0D221(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d);   //重庆规约

#endif /*__INCafn0dH*/
