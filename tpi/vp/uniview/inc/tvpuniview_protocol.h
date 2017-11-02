#ifndef _TVPUNIVIEW_PROTOCOL_H_
#define _TVPUNIVIEW_PROTOCOL_H_

#define MAX_XML_LENS_1M    (1024*1024*1)
#define MAX_PIC_LENS_5M    (1024*1024*5)
#define MAX_PIC_NUM_4      (4)


#define HEART_BEATE_CMD    (101)
#define RT_RECORDS_CMD     (115)



//tvp uniview data
typedef struct
{
	/* ���������� */
    char szDevCode[32];
	
	/* ��¼ID�� */
	char szRecordID[17];

	/* ���ڱ�� */
	char szTollgateID[32];

	/* ����ʱ�� */
	char szPassTime[32];

	/* �ص���룬����Ϊ�գ����32λ */
	char szPlaceCode[32];

    /* ������� */
    int ulLaneID;

    /* ���ƺ��� */
    char szCarPlate[32];

    /* ������ɫ 0-��ɫ1-��ɫ 2-��ɫ 3-��ɫ 4-���� */
    int ulPlateColor;

	/* �������� */
	int ulPlateNum;

    /* ���� */
    int ulVehSpeed;

	/* ���� */
	int ulLimitedSpeed;

	/* ʶ��״̬ 0��ʶ��ɹ� 1�����ɹ� 2�������� */
	int ulIdentifyStatus;

    /* ʶ��ʱ�䣬��λ���� */
    int ulIdentifyTime;

    /* �������� 1������ 2���侯 3������ 4��˫��  5������ */
    char szPlateType[3];

    /* ������ɫ A���ף�B���ң�C���ƣ�D���ۣ�E���죬F���ϣ�G���̣�H������I���أ�J���ڣ�K���ȣ�L���࣬M������N�����ף�Z������ */
    char szVehBodyColor;

    /* ������ɫ��ǳ 0-δ֪��1-ǳ��2-�� */
    int ulVehColorDepth;

    /* ��ʻ���� 1-��������2-���򶫣�3-���򱱣�4-�����ϣ�5-������������6-�������ϣ�7-���������ϣ�8-�����򶫱� */
    int ulVehDirection;

    /* ��������λ������ */
    int ulVehLength;

    /* �������� 0-δ֪��1-С�ͳ���2-���ͳ���3-���ͳ���4-����*/
    int ulVehtype;

    /* �������� 0-����������1-�ǻ������� */
    int ulLaneType;

    /* ����Ʒ�� */
    char szVehBrand[3];

    /* ����������ɫ */
    int ulDressColor;

    /* Υ������ 0-���ڣ�1-���٣�4-����� */
    int ulTransgressType;

    /* ���ʱ�� */
    int ulRedLightTime;

    /* ����ͼƬ���� ����URL����*/
    int ulPicNum;

    /* �ɼ��豸���룬����Ϊ�գ����32λ */
    char szEquipmentCode[64];  

	/*Ӧ������*/
	int ulApplicationType;

	/*ȫ�ֺϳɱ�־��*/
	int ulGlobalComposeFlag;

    /* �����ֶ� */
    int aulReserved[4];
	
}str_tvpuniview_xml;



//tvp uniview data
typedef struct
{
	int  xml_lens;
	str_tvpuniview_xml xml;
	int  pic_num;
	int  pic_lens[MAX_PIC_NUM_4];
	char pic_data[MAX_PIC_NUM_4][MAX_PIC_LENS_5M];
}str_tvpuniview_data;

//tvp uniview heart beate
typedef struct
{
	char dev_code[32];
}str_tvpuniview_heartbeate;

//tvpuniview frame 
typedef struct
{
	//------------
	unsigned long frame_head;
	unsigned long frame_tail;
	unsigned long frame_lens;
	unsigned long version;
	unsigned long frame_cmd;

	str_tvpuniview_data data;
	str_tvpuniview_heartbeate heart_beate;
}str_tvpuniview_protocol;



extern int tvpuniview_encode(str_tvpuniview_protocol *as_tvpuniview_protocol, char *oc_data);





#endif




