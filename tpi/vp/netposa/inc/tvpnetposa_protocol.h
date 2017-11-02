#ifndef _TVPNETPOSA_PROTOCOL_H_
#define _TVPNETPOSA_PROTOCOL_H_



extern int tvpnetposa_register_encode(char *oc_data, int ai_type);
extern int tvpnetposa_heartbeate_encode(char *oc_data, int ai_type);
extern int tvpnetposa_register_decode(char *ac_data, int ai_lens);
extern int tvpnetposa_heartbeate_decode(char *ac_data, int ai_lens);


#endif




