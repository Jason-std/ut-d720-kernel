#ifndef _URBETTER_CHECK_H_
#define _URBETTER_CHECK_H_

#include <linux/string.h>


#define UTCHECK(s1,s2)  (!strcmp(s1,s2))

extern char g_selected_utmodel[32];
#define CHECK_UTMODEL(str)  UTCHECK(g_selected_utmodel,str)

extern char g_selected_bltype[32] ;
#define CHECK_BLTYPE(str)  UTCHECK(g_selected_bltype,str)

extern char g_selected_codec[32];
#define CHECK_AUDIO(str) UTCHECK(g_selected_codec,str)

extern char g_selected_bt[32];
#define CHECK_BT(str) UTCHECK(g_selected_bt,str)

extern char g_selected_wifi[32];
#define CHECK_WIFI(str) UTCHECK(g_selected_wifi,str)

extern char g_selected_tp[32];
#define CHECK_TP(str) UTCHECK(g_selected_tp,str)

extern char g_selected_fm[32];
#define CHECK_FM(str) UTCHECK(g_selected_fm,str)

extern char g_selected_gsmd[32];
#define CHECK_GSM(str) UTCHECK(g_selected_gsmd,str)

extern char g_selected_nfc[32];
#define CHECK_NFC(str) UTCHECK(g_selected_nfc,str)

extern char g_selected_motor[32];
#define CHECK_MOTOR(str) UTCHECK(g_selected_motor,str)







#endif
